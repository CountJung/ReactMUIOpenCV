#pragma once

#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <filesystem>
#include <string>

namespace app {

class SettingsStore {
public:
  explicit SettingsStore(std::filesystem::path storage_path);

  nlohmann::json get() const;
  std::optional<nlohmann::json> update(const nlohmann::json& patch);

  static nlohmann::json defaults();

private:
  void load();
  void save_locked() const;

  std::filesystem::path storage_path_;
  mutable std::mutex mutex_;
  nlohmann::json settings_;
};

}  // namespace app
