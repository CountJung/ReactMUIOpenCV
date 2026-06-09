#include "PipelineStore.h"

#include "../common/Random.h"

namespace app {

nlohmann::json PipelineStore::list() const {
  std::scoped_lock lock(mutex_);
  auto result = nlohmann::json::array();
  for (const auto& [id, pipeline] : pipelines_) {
    result.push_back({{"id", id}, {"document", pipeline.document}, {"updatedAt", to_iso_time(pipeline.updated_at)}});
  }
  return {{"pipelines", result}};
}

nlohmann::json PipelineStore::create(const nlohmann::json& document) {
  PipelineRecord pipeline{random_hex(12), document, Clock::now()};
  std::scoped_lock lock(mutex_);
  pipelines_[pipeline.id] = pipeline;
  return {{"id", pipeline.id}, {"document", pipeline.document}};
}

nlohmann::json PipelineStore::replace(const std::string& id, const nlohmann::json& document) {
  std::scoped_lock lock(mutex_);
  pipelines_[id] = PipelineRecord{id, document, Clock::now()};
  return {{"id", id}, {"document", document}};
}

nlohmann::json PipelineStore::remove(const std::string& id) {
  std::scoped_lock lock(mutex_);
  pipelines_.erase(id);
  return {{"deleted", id}};
}

}  // namespace app
