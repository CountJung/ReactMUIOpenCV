#pragma once

#include "../common/Time.h"

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

namespace app {

struct PipelineRecord {
  std::string id;
  nlohmann::json document;
  Clock::time_point updated_at;
};

class PipelineStore {
 public:
  nlohmann::json list() const;
  nlohmann::json create(const nlohmann::json& document);
  nlohmann::json replace(const std::string& id, const nlohmann::json& document);
  nlohmann::json remove(const std::string& id);

 private:
  mutable std::mutex mutex_;
  std::map<std::string, PipelineRecord> pipelines_;
};

}  // namespace app
