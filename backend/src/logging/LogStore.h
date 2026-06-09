#pragma once

#include "../server/EventHub.h"

#include <nlohmann/json.hpp>

#include <deque>
#include <mutex>
#include <string>

namespace app {

class LogStore {
 public:
  explicit LogStore(EventHub& event_hub);

  nlohmann::json append(const std::string& level, const std::string& message);
  nlohmann::json recent() const;

 private:
  EventHub& event_hub_;
  mutable std::mutex mutex_;
  std::deque<nlohmann::json> entries_;
};

void configure_file_logging();

}  // namespace app
