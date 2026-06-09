#include "SettingsStore.h"

namespace app {

nlohmann::json SettingsStore::get() const {
  std::scoped_lock lock(mutex_);
  return {{"themeMode", theme_mode_}};
}

std::optional<nlohmann::json> SettingsStore::update_theme_mode(const std::string& theme_mode) {
  if (theme_mode != "light" && theme_mode != "dark" && theme_mode != "system") {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  theme_mode_ = theme_mode;
  return nlohmann::json{{"themeMode", theme_mode_}};
}

}  // namespace app
