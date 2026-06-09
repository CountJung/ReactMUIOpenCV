#pragma once

#include "../logging/LogStore.h"
#include "EventHub.h"

#include <ixwebsocket/IXWebSocketServer.h>

#include <string>

namespace app {

class WebSocketGateway {
 public:
  WebSocketGateway(std::string host, int port, EventHub& event_hub, LogStore& log_store);
  ~WebSocketGateway();

  WebSocketGateway(const WebSocketGateway&) = delete;
  WebSocketGateway& operator=(const WebSocketGateway&) = delete;

  bool start();
  void stop();

 private:
  std::string host_;
  int port_;
  EventHub& event_hub_;
  LogStore& log_store_;
  ix::WebSocketServer server_;
  bool net_initialized_ = false;
  bool started_ = false;
};

}  // namespace app
