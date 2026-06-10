#include "WebSocketGateway.h"

#include "../common/HttpUtils.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

namespace app {

namespace {

std::string query_value(const std::string& uri, const std::string& name) {
  const auto query_start = uri.find('?');
  if (query_start == std::string::npos) {
    return {};
  }

  const auto query = uri.substr(query_start + 1);
  const auto key = name + "=";
  std::size_t offset = 0;
  while (offset < query.size()) {
    const auto next = query.find('&', offset);
    const auto part = query.substr(offset, next == std::string::npos ? std::string::npos : next - offset);
    if (part.rfind(key, 0) == 0) {
      return part.substr(key.size());
    }
    if (next == std::string::npos) {
      break;
    }
    offset = next + 1;
  }

  return {};
}

bool is_loopback_ip(const std::string& remote_ip) {
  return remote_ip == "127.0.0.1" || remote_ip == "::1" ||
         remote_ip.rfind("::ffff:127.", 0) == 0 || remote_ip.empty();
}

}  // namespace

WebSocketGateway::WebSocketGateway(
    std::string host,
    int port,
    EventHub& event_hub,
    LogStore& log_store,
    RemoteAccessManager& remote_access)
    : host_(std::move(host)),
      port_(port),
      event_hub_(event_hub),
      log_store_(log_store),
      remote_access_(remote_access),
      server_(port_, host_) {}

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
          if (!is_loopback_ip(remote_ip)) {
            auto session_token = query_value(message->openInfo.uri, "sessionToken");
            const auto header = message->openInfo.headers.find("X-Remote-Session");
            if (session_token.empty() && header != message->openInfo.headers.end()) {
              session_token = header->second;
            }

            if (!remote_access_.permission_for_session(remote_ip, session_token)) {
              log_store_.append("warning", "Rejected unauthenticated WebSocket client from " + remote_ip);
              web_socket.close(1008, "Remote WebSocket session is not authenticated.");
              return;
            }
          }

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
  event_hub_.detach(&server_);
  if (started_) {
    server_.stop();
    started_ = false;
  }
}

}  // namespace app
