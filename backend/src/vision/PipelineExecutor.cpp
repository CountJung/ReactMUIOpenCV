#include "PipelineExecutor.h"

#include "../common/Random.h"
#include "../common/Time.h"

#include <stdexcept>

namespace app {

namespace {

std::string json_string(const nlohmann::json& value, const std::string& fallback = {}) {
  return value.is_string() ? value.get<std::string>() : fallback;
}

std::string node_id(const nlohmann::json& node) {
  return json_string(node.value("id", nlohmann::json{}), "unknown-node");
}

std::string node_type(const nlohmann::json& node) {
  return json_string(node.value("type", nlohmann::json{}), "operation");
}

const nlohmann::json& node_data(const nlohmann::json& node) {
  static const nlohmann::json empty = nlohmann::json::object();
  const auto found = node.find("data");
  return found != node.end() && found->is_object() ? *found : empty;
}

nlohmann::json step_payload(const std::string& node_id, const std::string& type, const std::string& status) {
  return {{"nodeId", node_id}, {"nodeType", type}, {"status", status}, {"timestamp", to_iso_time(Clock::now())}};
}

}  // namespace

PipelineExecutor::PipelineExecutor(ImageResultStore& image_store, EventHub& event_hub, JobQueue& job_queue, LogStore& log_store)
    : image_store_(image_store), event_hub_(event_hub), job_queue_(job_queue), log_store_(log_store) {}

nlohmann::json PipelineExecutor::execute(const nlohmann::json& document) {
  const auto job = job_queue_.create_manual("pipeline", "Pipeline execution queued.");
  const std::string job_id = job.value("id", std::string{});
  const std::string execution_id = random_hex(12);
  auto steps = nlohmann::json::array();
  std::string active_node_id;
  std::string active_node_type;

  try {
    const auto ordered_nodes = order_nodes(document);
    if (ordered_nodes.empty()) {
      throw std::runtime_error("Pipeline must contain at least one image input node.");
    }

    job_queue_.start(job_id, "Pipeline execution started.");
    std::string current_result_id;
    nlohmann::json final_result;

    for (std::size_t index = 0; index < ordered_nodes.size(); ++index) {
      const auto& node = ordered_nodes[index];
      active_node_id = node_id(node);
      active_node_type = node_type(node);
      const auto& data = node_data(node);
      publish_node_event("pipeline.node.started", job_id, node);

      nlohmann::json step = step_payload(active_node_id, active_node_type, "completed");
      if (active_node_type == "imageInput") {
        current_result_id = json_string(data.value("resultId", nlohmann::json{}));
        if (current_result_id.empty()) {
          throw std::runtime_error("Image input node requires data.resultId.");
        }

        auto result = image_store_.get(current_result_id);
        if (!result) {
          throw std::runtime_error("Image input result was not found.");
        }
        final_result = *result;
        step["result"] = final_result;
      } else if (active_node_type == "operation") {
        if (current_result_id.empty()) {
          throw std::runtime_error("Operation node requires an upstream image result.");
        }

        const auto operation = json_string(data.value("operation", nlohmann::json{}), "grayscale");
        const auto params = data.contains("params") && data["params"].is_object() ? data["params"] : nlohmann::json::object();
        final_result = image_store_.process(current_result_id, operation, params);
        current_result_id = final_result.value("resultId", std::string{});
        step["operation"] = operation;
        step["params"] = params;
        step["result"] = final_result;
        event_hub_.publish("preview.image.updated", {{"resultId", current_result_id}, {"source", "pipeline"}});
      } else if (active_node_type == "output") {
        if (current_result_id.empty()) {
          throw std::runtime_error("Output node requires an upstream image result.");
        }

        auto result = image_store_.get(current_result_id);
        if (!result) {
          throw std::runtime_error("Output image result was not found.");
        }
        final_result = *result;
        step["result"] = final_result;
      } else {
        throw std::runtime_error("Unsupported pipeline node type: " + active_node_type);
      }

      steps.push_back(step);
      publish_node_event("pipeline.node.completed", job_id, node, {{"resultId", current_result_id}});
      const auto progress = static_cast<int>(((index + 1) * 100) / ordered_nodes.size());
      job_queue_.progress(job_id, progress, "Pipeline node completed: " + active_node_id);
    }

    const auto completed_job = job_queue_.complete(job_id, "Pipeline execution completed.");
    log_store_.append("info", "Pipeline execution " + execution_id + " completed.");
    return {
        {"executionId", execution_id},
        {"job", completed_job.value_or(job)},
        {"status", "completed"},
        {"steps", steps},
        {"result", final_result},
        {"completedAt", to_iso_time(Clock::now())},
    };
  } catch (const std::exception& error) {
    if (!active_node_id.empty()) {
      event_hub_.publish(
          "pipeline.node.failed",
          {{"jobId", job_id}, {"nodeId", active_node_id}, {"nodeType", active_node_type}, {"message", error.what()}});
    }
    const auto failed_job = job_queue_.fail(job_id, error.what());
    return {
        {"executionId", execution_id},
        {"job", failed_job.value_or(job)},
        {"status", "failed"},
        {"steps", steps},
        {"error", error.what()},
        {"completedAt", to_iso_time(Clock::now())},
    };
  }
}

std::vector<nlohmann::json> PipelineExecutor::order_nodes(const nlohmann::json& document) const {
  if (!document.is_object() || !document.contains("nodes") || !document["nodes"].is_array()) {
    throw std::runtime_error("Pipeline document must contain a nodes array.");
  }

  const auto& nodes = document["nodes"];
  const auto& edges = document.contains("edges") && document["edges"].is_array() ? document["edges"] : nlohmann::json::array();
  std::map<std::string, nlohmann::json> by_id;
  std::map<std::string, std::string> next_by_source;
  std::string start_id;

  for (const auto& node : nodes) {
    const auto id = node_id(node);
    by_id[id] = node;
    if (node_type(node) == "imageInput" && start_id.empty()) {
      start_id = id;
    }
  }

  for (const auto& edge : edges) {
    const auto source = json_string(edge.value("source", nlohmann::json{}));
    const auto target = json_string(edge.value("target", nlohmann::json{}));
    if (!source.empty() && !target.empty() && !next_by_source.contains(source)) {
      next_by_source[source] = target;
    }
  }

  if (start_id.empty()) {
    return {};
  }

  std::vector<nlohmann::json> ordered;
  std::map<std::string, bool> visited;
  auto cursor = start_id;
  while (!cursor.empty()) {
    if (visited[cursor]) {
      throw std::runtime_error("Pipeline contains a cycle.");
    }
    visited[cursor] = true;

    const auto found = by_id.find(cursor);
    if (found == by_id.end()) {
      throw std::runtime_error("Pipeline edge references a missing node.");
    }
    ordered.push_back(found->second);

    const auto next = next_by_source.find(cursor);
    cursor = next == next_by_source.end() ? std::string{} : next->second;
  }

  return ordered;
}

void PipelineExecutor::publish_node_event(
    const std::string& type,
    const std::string& job_id,
    const nlohmann::json& node,
    const nlohmann::json& payload) const {
  auto event_payload = payload.is_object() ? payload : nlohmann::json::object();
  event_payload["jobId"] = job_id;
  event_payload["nodeId"] = node_id(node);
  event_payload["nodeType"] = node_type(node);
  event_hub_.publish(type, event_payload);
}

}  // namespace app
