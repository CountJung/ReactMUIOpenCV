#pragma once

#include "../image/ImageResultStore.h"
#include "../jobs/JobQueue.h"
#include "../logging/LogStore.h"
#include "../server/EventHub.h"

#include <nlohmann/json.hpp>

#include <map>
#include <string>
#include <vector>

namespace app {

class PipelineExecutor {
 public:
  PipelineExecutor(ImageResultStore& image_store, EventHub& event_hub, JobQueue& job_queue, LogStore& log_store);

  nlohmann::json execute(const nlohmann::json& document);

 private:
  std::vector<nlohmann::json> order_nodes(const nlohmann::json& document) const;
  void publish_node_event(const std::string& type, const std::string& job_id, const nlohmann::json& node, const nlohmann::json& payload = {}) const;

  ImageResultStore& image_store_;
  EventHub& event_hub_;
  JobQueue& job_queue_;
  LogStore& log_store_;
};

}  // namespace app
