#pragma once

#include "../common/Time.h"

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace app {

struct PipelineRecord {
  std::string id;
  nlohmann::json document;
  Clock::time_point updated_at;
};

class PipelineStore {
public:
  explicit PipelineStore(std::filesystem::path storage_path);

  nlohmann::json list(bool include_storage_path) const;
  nlohmann::json create(const nlohmann::json& document);
  nlohmann::json replace(const std::string& id, const nlohmann::json& document);
  nlohmann::json remove(const std::string& id);
  nlohmann::json record_execution(const nlohmann::json& execution);

private:
  void load();
  void save_locked() const;

  std::filesystem::path storage_path_;
  mutable std::mutex mutex_;
  std::map<std::string, PipelineRecord> pipelines_;
  std::vector<nlohmann::json> recent_executions_;
};

}  // namespace app
