#pragma once

#include "../logging/LogStore.h"
#include "../security/RemoteAccessManager.h"
#include "EventHub.h"

#include <ixwebsocket/IXWebSocketServer.h>

#include <string>

namespace app {

class WebSocketGateway {
 public:
  WebSocketGateway(
      std::string host,
      int port,
      EventHub& event_hub,
      LogStore& log_store,
      RemoteAccessManager& remote_access);
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
  RemoteAccessManager& remote_access_;
  ix::WebSocketServer server_;
  bool net_initialized_ = false;
  bool started_ = false;
};

}  // namespace app
