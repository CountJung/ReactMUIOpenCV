#include "WebSocketGateway.h"

#include "../common/HttpUtils.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

namespace app {

WebSocketGateway::WebSocketGateway(std::string host, int port, EventHub& event_hub, LogStore& log_store)
    : host_(std::move(host)), port_(port), event_hub_(event_hub), log_store_(log_store), server_(port_, host_) {}

WebSocketGateway::~WebSocketGateway() {
  stop();
  if (net_initialized_) {
    ix::uninitNetSystem();
  }
}

bool WebSocketGateway::start() {
  ix::initNetSystem();
  net_initialized_ = true;
  event_hub_.attach(&server_);

  server_.setOnClientMessageCallback(
      [&](std::shared_ptr<ix::ConnectionState> connection_state,
          ix::WebSocket& web_socket,
          const ix::WebSocketMessagePtr& message) {
        if (message->type == ix::WebSocketMessageType::Open) {
          const auto remote_ip = connection_state ? connection_state->getRemoteIp() : "unknown";
          log_store_.append("info", "WebSocket client connected from " + remote_ip);
          web_socket.send(event_hub_.publish("backend.status.changed", {{"websocket", "connected"}}).dump());
          web_socket.send(api_ok({{"events", event_hub_.recent()}}).dump());
        } else if (message->type == ix::WebSocketMessageType::Message) {
          web_socket.send(api_ok({{"type", "ack"}, {"received", message->str}}).dump());
        } else if (message->type == ix::WebSocketMessageType::Close) {
          log_store_.append("info", "WebSocket client disconnected");
        }
      });

  const auto listen_result = server_.listen();
  if (!listen_result.first) {
    log_store_.append("error", "WebSocket server failed to listen: " + listen_result.second);
    return false;
  }

  server_.start();
  started_ = true;
  log_store_.append("info", "WebSocket server listening on ws://" + host_ + ":" + std::to_string(port_));
  return true;
}

void WebSocketGateway::stop() {
  if (started_) {
    server_.stop();
    started_ = false;
  }
}

}  // namespace app
