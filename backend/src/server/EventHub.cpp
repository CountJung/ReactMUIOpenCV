#include "EventHub.h"

#include "../common/Constants.h"
#include "../common/Random.h"
#include "../common/Time.h"

namespace app {

void EventHub::attach(ix::WebSocketServer* server) {
  std::scoped_lock lock(mutex_);
  websocket_server_ = server;
}

nlohmann::json EventHub::publish(const std::string& type, const nlohmann::json& payload) {
  const nlohmann::json event = {
      {"id", random_hex(12)},
      {"type", type},
      {"timestamp", to_iso_time(Clock::now())},
      {"payload", payload},
  };

  std::scoped_lock lock(mutex_);
  recent_events_.push_back(event);
  while (recent_events_.size() > kMaxRecentEvents) {
    recent_events_.pop_front();
  }

  if (websocket_server_ != nullptr) {
    const auto serialized = event.dump();
    for (const auto& client : websocket_server_->getClients()) {
      client->send(serialized);
    }
  }

  return event;
}

nlohmann::json EventHub::recent() const {
  std::scoped_lock lock(mutex_);
  auto events = nlohmann::json::array();
  for (const auto& event : recent_events_) {
    events.push_back(event);
  }
  return events;
}

}  // namespace app
