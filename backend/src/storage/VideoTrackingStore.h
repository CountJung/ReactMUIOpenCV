#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <vector>

namespace app {

class VideoTrackingStore {
 public:
  explicit VideoTrackingStore(std::filesystem::path storage_path);

  nlohmann::json list(bool include_storage_path) const;
  nlohmann::json record(const nlohmann::json& tracking);

 private:
  void load();
  void save_locked() const;

  std::filesystem::path storage_path_;
  mutable std::shared_mutex mutex_;
  std::vector<nlohmann::json> records_;
};

}  // namespace app
