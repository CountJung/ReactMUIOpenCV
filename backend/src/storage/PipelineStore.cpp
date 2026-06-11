#include "PipelineStore.h"

#include "../common/Random.h"

#include <fstream>

namespace app {

namespace {

nlohmann::json record_to_json(const PipelineRecord& pipeline) {
  return {{"id", pipeline.id}, {"document", pipeline.document}, {"updatedAt", to_iso_time(pipeline.updated_at)}};
}

}  // namespace

PipelineStore::PipelineStore(std::filesystem::path storage_path) : storage_path_(std::move(storage_path)) {
  load();
}

nlohmann::json PipelineStore::list(bool include_storage_path) const {
  std::scoped_lock lock(mutex_);
  auto result = nlohmann::json::array();
  for (const auto& [id, pipeline] : pipelines_) {
    result.push_back(record_to_json(pipeline));
  }

  nlohmann::json storage = {
      {"kind", "jsonFile"},
      {"description", "Saved pipeline documents are persisted by the local backend."},
  };
  if (include_storage_path) {
    storage["path"] = std::filesystem::absolute(storage_path_).lexically_normal().string();
  }

  return {{"pipelines", result}, {"executions", recent_executions_}, {"storage", storage}};
}

nlohmann::json PipelineStore::create(const nlohmann::json& document) {
  PipelineRecord pipeline{random_hex(12), document, Clock::now()};
  std::scoped_lock lock(mutex_);
  pipelines_[pipeline.id] = pipeline;
  save_locked();
  return record_to_json(pipeline);
}

nlohmann::json PipelineStore::replace(const std::string& id, const nlohmann::json& document) {
  std::scoped_lock lock(mutex_);
  pipelines_[id] = PipelineRecord{id, document, Clock::now()};
  save_locked();
  return record_to_json(pipelines_[id]);
}

nlohmann::json PipelineStore::remove(const std::string& id) {
  std::scoped_lock lock(mutex_);
  pipelines_.erase(id);
  save_locked();
  return {{"deleted", id}};
}

nlohmann::json PipelineStore::record_execution(const nlohmann::json& execution) {
  std::scoped_lock lock(mutex_);
  recent_executions_.insert(recent_executions_.begin(), execution);
  while (recent_executions_.size() > 20) {
    recent_executions_.pop_back();
  }
  save_locked();
  return execution;
}

void PipelineStore::load() {
  std::scoped_lock lock(mutex_);
  pipelines_.clear();
  recent_executions_.clear();

  std::ifstream input(storage_path_);
  if (!input) {
    return;
  }

  const auto payload = nlohmann::json::parse(input, nullptr, false);
  if (!payload.is_object()) {
    return;
  }

  const auto pipelines = payload.value("pipelines", nlohmann::json::array());
  if (pipelines.is_array()) {
    for (const auto& item : pipelines) {
      const auto id = item.value("id", std::string{});
      if (id.empty() || !item.contains("document")) {
        continue;
      }
      pipelines_[id] = PipelineRecord{id, item["document"], Clock::now()};
    }
  }

  const auto executions = payload.value("executions", nlohmann::json::array());
  if (executions.is_array()) {
    for (const auto& execution : executions) {
      recent_executions_.push_back(execution);
      if (recent_executions_.size() >= 20) {
        break;
      }
    }
  }
}

void PipelineStore::save_locked() const {
  std::filesystem::create_directories(storage_path_.parent_path());
  auto pipelines = nlohmann::json::array();
  for (const auto& [_, pipeline] : pipelines_) {
    pipelines.push_back(record_to_json(pipeline));
  }

  const nlohmann::json payload = {
      {"version", 1},
      {"pipelines", pipelines},
      {"executions", recent_executions_},
  };

  const auto temp_path = storage_path_;
  std::ofstream output(temp_path, std::ios::binary | std::ios::trunc);
  output << payload.dump(2);
}

}  // namespace app
