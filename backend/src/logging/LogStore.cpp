#include "LogStore.h"

#include "../common/Constants.h"
#include "../common/Random.h"
#include "../common/Time.h"

#include <filesystem>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace app {

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

void configure_file_logging() {
  std::filesystem::create_directories("logs");
  auto logger = spdlog::basic_logger_mt("backend", "logs/backend.log", true);
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
  spdlog::flush_on(spdlog::level::info);
}

}  // namespace app
