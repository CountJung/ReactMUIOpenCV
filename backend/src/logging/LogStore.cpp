#include "LogStore.h"

#include "../common/Constants.h"
#include "../common/Random.h"
#include "../common/Time.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace app {

namespace {

constexpr int kDefaultLogRetentionDays = 5;
constexpr int kDefaultLogMaxFileMb = 10;
constexpr int kMinLogRetentionDays = 1;
constexpr int kMaxLogRetentionDays = 30;
constexpr int kMinLogMaxFileMb = 1;
constexpr int kMaxLogMaxFileMb = 100;

std::mutex& logging_config_mutex() {
  static std::mutex mutex;
  return mutex;
}

int clamped_int_setting(const nlohmann::json& settings, const char* key, int fallback, int min, int max) {
  try {
    return std::clamp(settings.value(key, fallback), min, max);
  } catch (const std::exception&) {
    return fallback;
  }
}

spdlog::level::level_enum log_level_from_settings(const nlohmann::json& settings) {
  const auto level = settings.value("level", std::string{"info"});
  if (level == "debug") {
    return spdlog::level::debug;
  }
  if (level == "warning") {
    return spdlog::level::warn;
  }
  if (level == "error") {
    return spdlog::level::err;
  }
  return spdlog::level::info;
}

void cleanup_old_logs(const std::filesystem::path& log_dir, int retention_days) {
  if (!std::filesystem::exists(log_dir)) {
    return;
  }

  const auto now = std::filesystem::file_time_type::clock::now();
  const auto max_age = std::chrono::hours(24 * retention_days);
  for (const auto& entry : std::filesystem::directory_iterator(log_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto path = entry.path();
    if (path.extension() != ".log" && path.filename().string().find("backend") == std::string::npos) {
      continue;
    }
    const auto age = now - entry.last_write_time();
    if (age > max_age) {
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  }
}

nlohmann::json logging_settings_from_file(const std::filesystem::path& settings_path) {
  std::ifstream input(settings_path);
  if (!input) {
    return nlohmann::json::object();
  }

  const auto parsed = nlohmann::json::parse(input, nullptr, false);
  if (!parsed.is_object()) {
    return nlohmann::json::object();
  }

  return parsed.value("logging", nlohmann::json::object());
}

}  // namespace

LogStore::LogStore(EventHub& event_hub)
    : event_hub_(event_hub) {}

nlohmann::json LogStore::append(const std::string& level, const std::string& message) {
  const nlohmann::json entry = {
      {"id", random_hex(12)},
      {"timestamp", to_iso_time(Clock::now())},
      {"level", level},
      {"message", message},
  };

  {
    std::scoped_lock lock(mutex_);
    entries_.push_back(entry);
    while (entries_.size() > kMaxRecentLogs) {
      entries_.pop_front();
    }
  }

  if (level == "error") {
    spdlog::error(message);
  } else if (level == "warning") {
    spdlog::warn(message);
  } else {
    spdlog::info(message);
  }

  event_hub_.publish("log.appended", entry);
  return entry;
}

nlohmann::json LogStore::recent() const {
  std::scoped_lock lock(mutex_);
  auto logs = nlohmann::json::array();
  for (const auto& entry : entries_) {
    logs.push_back(entry);
  }
  return {{"logs", logs}};
}

void configure_file_logging(const nlohmann::json& settings) {
  std::scoped_lock lock(logging_config_mutex());

  const auto logging_settings =
      settings.contains("logging") && settings["logging"].is_object() ? settings["logging"] : settings;
  const auto retention_days = clamped_int_setting(
      logging_settings, "retentionDays", kDefaultLogRetentionDays, kMinLogRetentionDays, kMaxLogRetentionDays);
  const auto max_file_mb =
      clamped_int_setting(logging_settings, "maxFileMb", kDefaultLogMaxFileMb, kMinLogMaxFileMb, kMaxLogMaxFileMb);
  const auto max_size = static_cast<std::size_t>(max_file_mb) * 1024ULL * 1024ULL;
  const auto max_files = static_cast<std::size_t>(retention_days);

  const std::filesystem::path log_dir = "logs";
  std::filesystem::create_directories(log_dir);
  cleanup_old_logs(log_dir, retention_days);

  const auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
  spdlog::set_default_logger(std::make_shared<spdlog::logger>("backend-reconfigure", null_sink));
  spdlog::drop("backend");

  auto logger = spdlog::rotating_logger_mt("backend", (log_dir / "backend.log").string(), max_size, max_files, false);
  logger->set_level(log_level_from_settings(logging_settings));
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
  spdlog::flush_on(spdlog::level::info);
}

void configure_file_logging_from_settings_file(const std::filesystem::path& settings_path) {
  configure_file_logging(logging_settings_from_file(settings_path));
}

}  // namespace app
