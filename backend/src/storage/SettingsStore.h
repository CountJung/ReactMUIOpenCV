#pragma once

#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace app {

class SettingsStore {
 public:
  nlohmann::json get() const;
  std::optional<nlohmann::json> update_theme_mode(const std::string& theme_mode);

 private:
  mutable std::mutex mutex_;
  std::string theme_mode_ = "system";
};

}  // namespace app
