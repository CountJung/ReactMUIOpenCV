#pragma once

#include "../common/Time.h"

#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace app {

struct RemoteClient {
  std::string id;
  std::string address;
  std::string permission;
  std::string session_token;
  Clock::time_point connected_at;
  Clock::time_point expires_at;
};

nlohmann::json client_to_json(const RemoteClient& client);

class RemoteAccessManager {
 public:
  RemoteAccessManager(bool lan_bound, std::string bind_host, std::string selected_ip);

  nlohmann::json status(int port);
  nlohmann::json enable(int port);
  nlohmann::json disable(int port);
  nlohmann::json rotate_token(int port);
  nlohmann::json clients();
  nlohmann::json disconnect_all();
  std::optional<nlohmann::json> authenticate(const std::string& remote_addr, const std::string& token, const std::string& pin);

  bool enabled() const;
  bool lan_bound() const;
  std::string bind_host() const;
  std::string selected_ip() const;

 private:
  void prune_expired_clients();
  nlohmann::json status_locked(int port);

  mutable std::mutex mutex_;
  bool lan_bound_ = false;
  bool enabled_ = false;
  std::string bind_host_ = "127.0.0.1";
  std::string selected_ip_ = "127.0.0.1";
  std::string access_token_;
  std::string pin_;
  std::vector<RemoteClient> clients_;
};

}  // namespace app
