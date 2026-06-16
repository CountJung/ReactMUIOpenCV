#pragma once

#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <vector>

namespace app {

class PerformanceBenchmarkStore {
public:
  explicit PerformanceBenchmarkStore(std::filesystem::path storage_path);

  nlohmann::json list(bool include_storage_path) const;
  nlohmann::json record(const nlohmann::json& benchmark);

private:
  void load();
  void save_locked() const;

  std::filesystem::path storage_path_;
  mutable std::mutex mutex_;
  std::vector<nlohmann::json> records_;
};

}  // namespace app
