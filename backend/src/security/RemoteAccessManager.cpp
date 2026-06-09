#include "RemoteAccessManager.h"

#include "../common/Constants.h"
#include "../common/Random.h"

#include <algorithm>
#include <chrono>
#include <optional>

namespace app {

nlohmann::json client_to_json(const RemoteClient& client) {
  return {
      {"id", client.id},
      {"address", client.address},
      {"permission", client.permission},
      {"connectedAt", to_iso_time(client.connected_at)},
      {"expiresAt", to_iso_time(client.expires_at)},
  };
}

RemoteAccessManager::RemoteAccessManager(bool lan_bound, std::string bind_host, std::string selected_ip)
    : lan_bound_(lan_bound),
      enabled_(lan_bound),
      bind_host_(std::move(bind_host)),
      selected_ip_(std::move(selected_ip)),
      access_token_(random_hex(32)),
      pin_(random_pin()) {}

nlohmann::json RemoteAccessManager::status(int port) {
  std::scoped_lock lock(mutex_);
  return status_locked(port);
}

nlohmann::json RemoteAccessManager::enable(int port) {
  std::scoped_lock lock(mutex_);
  enabled_ = true;
  access_token_ = random_hex(32);
  pin_ = random_pin();
  return status_locked(port);
}

nlohmann::json RemoteAccessManager::disable(int port) {
  std::scoped_lock lock(mutex_);
  enabled_ = false;
  clients_.clear();
  return status_locked(port);
}

nlohmann::json RemoteAccessManager::rotate_token(int port) {
  std::scoped_lock lock(mutex_);
  access_token_ = random_hex(32);
  pin_ = random_pin();
  clients_.clear();
  return status_locked(port);
}

nlohmann::json RemoteAccessManager::clients() {
  std::scoped_lock lock(mutex_);
  prune_expired_clients();

  auto result = nlohmann::json::array();
  for (const auto& client : clients_) {
    result.push_back(client_to_json(client));
  }

  return {{"clients", result}};
}

nlohmann::json RemoteAccessManager::disconnect_all() {
  std::scoped_lock lock(mutex_);
  clients_.clear();
  return {{"clients", nlohmann::json::array()}, {"message", "All remote clients disconnected."}};
}

std::optional<nlohmann::json> RemoteAccessManager::authenticate(
    const std::string& remote_addr,
    const std::string& token,
    const std::string& pin) {
  std::scoped_lock lock(mutex_);
  prune_expired_clients();

  if (!enabled_ || (token != access_token_ && pin != pin_)) {
    return std::nullopt;
  }

  const auto now = Clock::now();
  RemoteClient client{
      random_hex(12),
      remote_addr.empty() ? "unknown" : remote_addr,
      "read-only",
      random_hex(32),
      now,
      now + std::chrono::minutes(kSessionTimeoutMinutes),
  };

  clients_.push_back(client);
  return nlohmann::json{
      {"sessionToken", client.session_token},
      {"permission", "read-only"},
      {"expiresAt", to_iso_time(client.expires_at)},
      {"client", client_to_json(client)},
  };
}

bool RemoteAccessManager::enabled() const {
  std::scoped_lock lock(mutex_);
  return enabled_;
}

bool RemoteAccessManager::lan_bound() const {
  std::scoped_lock lock(mutex_);
  return lan_bound_;
}

std::string RemoteAccessManager::bind_host() const {
  std::scoped_lock lock(mutex_);
  return bind_host_;
}

std::string RemoteAccessManager::selected_ip() const {
  std::scoped_lock lock(mutex_);
  return selected_ip_;
}

void RemoteAccessManager::prune_expired_clients() {
  const auto now = Clock::now();
  clients_.erase(
      std::remove_if(clients_.begin(), clients_.end(), [now](const RemoteClient& client) {
        return client.expires_at <= now;
      }),
      clients_.end());
}

nlohmann::json RemoteAccessManager::status_locked(int port) {
  prune_expired_clients();
  const std::string local_url = "http://127.0.0.1:" + std::to_string(port);
  const std::string lan_url = "http://" + selected_ip_ + ":" + std::to_string(port);
  const std::string connection_url = lan_url + "?token=" + access_token_;

  auto client_list = nlohmann::json::array();
  for (const auto& client : clients_) {
    client_list.push_back(client_to_json(client));
  }

  return {
      {"enabled", enabled_},
      {"lanBound", lan_bound_},
      {"bindHost", bind_host_},
      {"selectedIp", selected_ip_},
      {"port", port},
      {"localUrl", local_url},
      {"lanUrl", lan_url},
      {"connectionUrl", connection_url},
      {"accessToken", access_token_},
      {"pin", pin_},
      {"sessionTimeoutMinutes", kSessionTimeoutMinutes},
      {"defaultPermission", "read-only"},
      {"clients", client_list},
  };
}

}  // namespace app
