#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include <nlohmann/json.hpp>

#include <deque>
#include <mutex>
#include <string>

namespace app {

class EventHub {
 public:
  void attach(ix::WebSocketServer* server);
  void detach(ix::WebSocketServer* server);
  nlohmann::json publish(const std::string& type, const nlohmann::json& payload);
  nlohmann::json recent() const;

 private:
  mutable std::mutex mutex_;
  std::deque<nlohmann::json> recent_events_;
  ix::WebSocketServer* websocket_server_ = nullptr;
};

}  // namespace app
