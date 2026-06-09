#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <httplib.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <nlohmann/json.hpp>
#include <opencv2/core/version.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace {

constexpr int kDefaultPort = 18730;
constexpr int kDefaultWsPort = 18731;
constexpr int kSessionTimeoutMinutes = 30;
constexpr std::size_t kMaxRecentEvents = 200;
constexpr std::size_t kMaxRecentLogs = 300;

using Clock = std::chrono::system_clock;

struct RemoteClient {
  std::string id;
  std::string address;
  std::string permission;
  std::string session_token;
  Clock::time_point connected_at;
  Clock::time_point expires_at;
};

struct RemoteState {
  bool lan_bound = false;
  bool enabled = false;
  std::string bind_host = "127.0.0.1";
  std::string selected_ip = "127.0.0.1";
  std::string access_token;
  std::string pin;
  std::vector<RemoteClient> clients;
  std::mutex mutex;
};

struct SettingsState {
  std::string theme_mode = "system";
  std::mutex mutex;
};

struct PipelineRecord {
  std::string id;
  nlohmann::json document;
  Clock::time_point updated_at;
};

bool has_arg(int argc, char* argv[], const std::string& value) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == value) {
      return true;
    }
  }

  return false;
}

std::string random_hex(std::size_t length) {
  static constexpr std::array<char, 16> kHex = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  static std::random_device random_device;
  static std::mt19937 generator(random_device());
  static std::uniform_int_distribution<int> distribution(0, 15);

  std::string value;
  value.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    value.push_back(kHex[distribution(generator)]);
  }

  return value;
}

std::string random_pin() {
  static std::random_device random_device;
  static std::mt19937 generator(random_device());
  static std::uniform_int_distribution<int> distribution(0, 999999);

  std::ostringstream stream;
  stream << std::setw(6) << std::setfill('0') << distribution(generator);
  return stream.str();
}

std::string to_iso_time(Clock::time_point time_point) {
  const auto time = Clock::to_time_t(time_point);
  std::tm utc{};

#ifdef _WIN32
  gmtime_s(&utc, &time);
#else
  gmtime_r(&time, &utc);
#endif

  std::ostringstream stream;
  stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  return stream.str();
}

bool is_private_ipv4(const std::string& ip) {
  return ip.rfind("10.", 0) == 0 || ip.rfind("192.168.", 0) == 0 ||
         ip.rfind("172.16.", 0) == 0 || ip.rfind("172.17.", 0) == 0 ||
         ip.rfind("172.18.", 0) == 0 || ip.rfind("172.19.", 0) == 0 ||
         ip.rfind("172.20.", 0) == 0 || ip.rfind("172.21.", 0) == 0 ||
         ip.rfind("172.22.", 0) == 0 || ip.rfind("172.23.", 0) == 0 ||
         ip.rfind("172.24.", 0) == 0 || ip.rfind("172.25.", 0) == 0 ||
         ip.rfind("172.26.", 0) == 0 || ip.rfind("172.27.", 0) == 0 ||
         ip.rfind("172.28.", 0) == 0 || ip.rfind("172.29.", 0) == 0 ||
         ip.rfind("172.30.", 0) == 0 || ip.rfind("172.31.", 0) == 0;
}

std::vector<std::string> get_lan_ipv4_addresses() {
#ifdef _WIN32
  WSADATA wsa_data{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return {};
  }
#endif

  char hostname[256]{};
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    return {};
  }

  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
    return {};
  }

  std::set<std::string> addresses;
  for (addrinfo* current = result; current != nullptr; current = current->ai_next) {
    char buffer[INET_ADDRSTRLEN]{};
    auto* ipv4 = reinterpret_cast<sockaddr_in*>(current->ai_addr);
    if (inet_ntop(AF_INET, &(ipv4->sin_addr), buffer, sizeof(buffer)) != nullptr) {
      const std::string ip = buffer;
      if (ip != "127.0.0.1" && ip.rfind("169.254.", 0) != 0) {
        addresses.insert(ip);
      }
    }
  }

  freeaddrinfo(result);

  std::vector<std::string> sorted(addresses.begin(), addresses.end());
  std::stable_sort(sorted.begin(), sorted.end(), [](const std::string& left, const std::string& right) {
    return is_private_ipv4(left) > is_private_ipv4(right);
  });

  return sorted;
}

nlohmann::json api_ok(const nlohmann::json& data = nlohmann::json::object()) {
  return {{"ok", true}, {"data", data}, {"error", nullptr}};
}

nlohmann::json api_error(const std::string& code, const std::string& message) {
  return {{"ok", false}, {"data", nullptr}, {"error", {{"code", code}, {"message", message}}}};
}

void set_cors(httplib::Response& response) {
  response.set_header("Access-Control-Allow-Origin", "*");
  response.set_header("Access-Control-Allow-Headers", "Content-Type, X-Remote-Session");
  response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
}

void send_json(httplib::Response& response, const nlohmann::json& body, int status = 200) {
  response.status = status;
  set_cors(response);
  response.set_content(body.dump(), "application/json");
}

void send_data(httplib::Response& response, const nlohmann::json& data, int status = 200) {
  send_json(response, api_ok(data), status);
}

void send_error(httplib::Response& response, const std::string& code, const std::string& message, int status) {
  send_json(response, api_error(code, message), status);
}

nlohmann::json parse_body(const httplib::Request& request) {
  if (request.body.empty()) {
    return nlohmann::json::object();
  }

  auto parsed = nlohmann::json::parse(request.body, nullptr, false);
  return parsed.is_discarded() ? nlohmann::json::object() : parsed;
}

class EventHub {
 public:
  void attach(ix::WebSocketServer* server) {
    std::scoped_lock lock(mutex_);
    websocket_server_ = server;
  }

  nlohmann::json publish(const std::string& type, const nlohmann::json& payload) {
    const nlohmann::json event = {
        {"id", random_hex(12)},
        {"type", type},
        {"timestamp", to_iso_time(Clock::now())},
        {"payload", payload},
    };

    {
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
    }

    return event;
  }

  nlohmann::json recent() const {
    std::scoped_lock lock(mutex_);
    auto events = nlohmann::json::array();
    for (const auto& event : recent_events_) {
      events.push_back(event);
    }
    return events;
  }

 private:
  mutable std::mutex mutex_;
  std::deque<nlohmann::json> recent_events_;
  ix::WebSocketServer* websocket_server_ = nullptr;
};

class LogStore {
 public:
  explicit LogStore(EventHub& event_hub) : event_hub_(event_hub) {}

  nlohmann::json append(const std::string& level, const std::string& message) {
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

  nlohmann::json recent() const {
    std::scoped_lock lock(mutex_);
    auto logs = nlohmann::json::array();
    for (const auto& entry : entries_) {
      logs.push_back(entry);
    }
    return {{"logs", logs}};
  }

 private:
  EventHub& event_hub_;
  mutable std::mutex mutex_;
  std::deque<nlohmann::json> entries_;
};

struct JobRecord {
  std::string id;
  std::string type;
  std::string status;
  int progress = 0;
  std::string message;
  Clock::time_point created_at;
  Clock::time_point updated_at;
};

nlohmann::json job_to_json(const JobRecord& job) {
  return {
      {"id", job.id},
      {"type", job.type},
      {"status", job.status},
      {"progress", job.progress},
      {"message", job.message},
      {"createdAt", to_iso_time(job.created_at)},
      {"updatedAt", to_iso_time(job.updated_at)},
  };
}

class JobQueue {
 public:
  JobQueue(EventHub& event_hub, LogStore& log_store) : event_hub_(event_hub), log_store_(log_store) {
    worker_ = std::thread([this] { run(); });
  }

  ~JobQueue() {
    {
      std::scoped_lock lock(mutex_);
      stopping_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }
  }

  nlohmann::json enqueue(const std::string& type, const std::string& message) {
    JobRecord job;
    job.id = random_hex(12);
    job.type = type;
    job.status = "queued";
    job.progress = 0;
    job.message = message;
    job.created_at = Clock::now();
    job.updated_at = job.created_at;

    {
      std::scoped_lock lock(mutex_);
      jobs_[job.id] = job;
      pending_.push(job.id);
    }

    log_store_.append("info", "Queued " + type + " job " + job.id);
    cv_.notify_one();
    return job_to_json(job);
  }

  nlohmann::json list() const {
    std::scoped_lock lock(mutex_);
    auto jobs = nlohmann::json::array();
    for (const auto& [_, job] : jobs_) {
      jobs.push_back(job_to_json(job));
    }
    return {{"jobs", jobs}};
  }

  std::optional<nlohmann::json> get(const std::string& id) const {
    std::scoped_lock lock(mutex_);
    const auto found = jobs_.find(id);
    if (found == jobs_.end()) {
      return std::nullopt;
    }

    return job_to_json(found->second);
  }

  bool cancel(const std::string& id) {
    std::scoped_lock lock(mutex_);
    auto found = jobs_.find(id);
    if (found == jobs_.end() || found->second.status == "completed") {
      return false;
    }

    found->second.status = "cancelled";
    found->second.message = "Cancelled by user.";
    found->second.updated_at = Clock::now();
    event_hub_.publish("job.cancelled", job_to_json(found->second));
    return true;
  }

  bool remove(const std::string& id) {
    std::scoped_lock lock(mutex_);
    return jobs_.erase(id) > 0;
  }

 private:
  void run() {
    while (true) {
      std::string job_id;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return stopping_ || !pending_.empty(); });

        if (stopping_) {
          return;
        }

        job_id = pending_.front();
        pending_.pop();

        auto& job = jobs_.at(job_id);
        if (job.status == "cancelled") {
          continue;
        }

        job.status = "running";
        job.message = "Processing started.";
        job.updated_at = Clock::now();
        event_hub_.publish("job.started", job_to_json(job));
      }

      for (int progress = 20; progress <= 100; progress += 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(180));

        std::scoped_lock lock(mutex_);
        auto& job = jobs_.at(job_id);
        if (job.status == "cancelled") {
          break;
        }

        job.progress = progress;
        job.updated_at = Clock::now();
        event_hub_.publish("job.progress", job_to_json(job));

        if (progress == 100) {
          job.status = "completed";
          job.message = "Processing completed.";
          job.updated_at = Clock::now();
          event_hub_.publish("job.completed", job_to_json(job));
          log_store_.append("info", "Completed " + job.type + " job " + job.id);
        }
      }
    }
  }

  EventHub& event_hub_;
  LogStore& log_store_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::map<std::string, JobRecord> jobs_;
  std::queue<std::string> pending_;
  std::thread worker_;
  bool stopping_ = false;
};

void prune_expired_clients(RemoteState& state) {
  const auto now = Clock::now();
  state.clients.erase(
      std::remove_if(state.clients.begin(), state.clients.end(), [now](const RemoteClient& client) {
        return client.expires_at <= now;
      }),
      state.clients.end());
}

nlohmann::json client_to_json(const RemoteClient& client) {
  return {
      {"id", client.id},
      {"address", client.address},
      {"permission", client.permission},
      {"connectedAt", to_iso_time(client.connected_at)},
      {"expiresAt", to_iso_time(client.expires_at)},
  };
}

nlohmann::json remote_status_to_json(RemoteState& state, int port) {
  prune_expired_clients(state);
  const std::string local_url = "http://127.0.0.1:" + std::to_string(port);
  const std::string lan_url = "http://" + state.selected_ip + ":" + std::to_string(port);
  const std::string connection_url = lan_url + "?token=" + state.access_token;

  return {
      {"enabled", state.enabled},
      {"lanBound", state.lan_bound},
      {"bindHost", state.bind_host},
      {"selectedIp", state.selected_ip},
      {"port", port},
      {"localUrl", local_url},
      {"lanUrl", lan_url},
      {"connectionUrl", connection_url},
      {"accessToken", state.access_token},
      {"pin", state.pin},
      {"sessionTimeoutMinutes", kSessionTimeoutMinutes},
      {"defaultPermission", "read-only"},
      {"clients", nlohmann::json::array()},
  };
}

void configure_file_logging() {
  std::filesystem::create_directories("logs");
  auto logger = spdlog::basic_logger_mt("backend", "logs/backend.log", true);
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
  spdlog::flush_on(spdlog::level::info);
}

}  // namespace

int main(int argc, char* argv[]) {
  configure_file_logging();

  const bool lan_mode = has_arg(argc, argv, "--lan");
  const std::string host = lan_mode ? "0.0.0.0" : "127.0.0.1";
  const int port = kDefaultPort;
  const int ws_port = kDefaultWsPort;
  const auto lan_addresses = get_lan_ipv4_addresses();

  EventHub event_hub;
  LogStore log_store(event_hub);
  JobQueue job_queue(event_hub, log_store);
  SettingsState settings_state;
  std::map<std::string, PipelineRecord> pipelines;
  std::mutex pipelines_mutex;

  RemoteState remote_state;
  remote_state.lan_bound = lan_mode;
  remote_state.enabled = lan_mode;
  remote_state.bind_host = host;
  remote_state.selected_ip = lan_addresses.empty() ? "127.0.0.1" : lan_addresses.front();
  remote_state.access_token = random_hex(32);
  remote_state.pin = random_pin();

  ix::initNetSystem();
  ix::WebSocketServer websocket_server(ws_port, host);
  event_hub.attach(&websocket_server);
  websocket_server.setOnClientMessageCallback(
      [&](std::shared_ptr<ix::ConnectionState> connection_state,
          ix::WebSocket& web_socket,
          const ix::WebSocketMessagePtr& message) {
        if (message->type == ix::WebSocketMessageType::Open) {
          const auto remote_ip = connection_state ? connection_state->getRemoteIp() : "unknown";
          log_store.append("info", "WebSocket client connected from " + remote_ip);
          web_socket.send(event_hub.publish("backend.status.changed", {{"websocket", "connected"}}).dump());
          web_socket.send(api_ok({{"events", event_hub.recent()}}).dump());
        } else if (message->type == ix::WebSocketMessageType::Message) {
          web_socket.send(api_ok({{"type", "ack"}, {"received", message->str}}).dump());
        } else if (message->type == ix::WebSocketMessageType::Close) {
          log_store.append("info", "WebSocket client disconnected");
        }
      });

  const auto ws_listen_result = websocket_server.listen();
  if (ws_listen_result.first) {
    websocket_server.start();
    log_store.append("info", "WebSocket server listening on ws://" + host + ":" + std::to_string(ws_port));
  } else {
    log_store.append("error", "WebSocket server failed to listen: " + ws_listen_result.second);
  }

  httplib::Server server;

  server.Options(".*", [](const httplib::Request&, httplib::Response& response) {
    set_cors(response);
  });

  server.Get("/api/health", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {
                            {"status", "ok"},
                            {"service", "ReactMUIOpenCV"},
                            {"opencvVersion", CV_VERSION},
                        });
  });

  server.Get("/api/server-info", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {
                            {"service", "ReactMUIOpenCV"},
                            {"httpUrl", "http://" + host + ":" + std::to_string(port)},
                            {"wsUrl", "ws://" + host + ":" + std::to_string(ws_port)},
                            {"opencvVersion", CV_VERSION},
                        });
  });

  server.Get("/api/network-info", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {
                            {"hostName", "localhost"},
                            {"bindHost", host},
                            {"port", port},
                            {"webSocketPort", ws_port},
                            {"lanBound", lan_mode},
                            {"selectedIp", remote_state.selected_ip},
                            {"addresses", lan_addresses},
                        });
  });

  server.Get("/api/settings", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(settings_state.mutex);
    send_data(response, {{"themeMode", settings_state.theme_mode}});
  });

  server.Put("/api/settings", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto next_mode = body.value("themeMode", std::string{"system"});
    if (next_mode != "light" && next_mode != "dark" && next_mode != "system") {
      send_error(response, "invalid_settings", "themeMode must be light, dark, or system.", 400);
      return;
    }

    {
      std::scoped_lock lock(settings_state.mutex);
      settings_state.theme_mode = next_mode;
    }

    log_store.append("info", "Settings updated");
    send_data(response, {{"themeMode", next_mode}});
  });

  server.Get("/api/remote/status", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(remote_state.mutex);
    auto body = remote_status_to_json(remote_state, port);
    body["clients"] = nlohmann::json::array();

    for (const auto& client : remote_state.clients) {
      body["clients"].push_back(client_to_json(client));
    }

    send_data(response, body);
  });

  server.Post("/api/remote/enable", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(remote_state.mutex);
    remote_state.enabled = true;
    remote_state.access_token = random_hex(32);
    remote_state.pin = random_pin();

    auto body = remote_status_to_json(remote_state, port);
    body["message"] = remote_state.lan_bound
                          ? "LAN Web UI is enabled."
                          : "Remote access is enabled, but restart with --lan to bind externally.";
    event_hub.publish("backend.status.changed", {{"remoteAccess", "enabled"}});
    log_store.append("warning", "LAN Web UI mode enabled");
    send_data(response, body);
  });

  server.Post("/api/remote/disable", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(remote_state.mutex);
    remote_state.enabled = false;
    remote_state.clients.clear();

    auto body = remote_status_to_json(remote_state, port);
    body["message"] = "LAN Web UI is disabled and clients were disconnected.";
    event_hub.publish("backend.status.changed", {{"remoteAccess", "disabled"}});
    log_store.append("info", "LAN Web UI mode disabled");
    send_data(response, body);
  });

  server.Post("/api/remote/rotate-token", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(remote_state.mutex);
    remote_state.access_token = random_hex(32);
    remote_state.pin = random_pin();
    remote_state.clients.clear();

    auto body = remote_status_to_json(remote_state, port);
    body["message"] = "Access token and PIN rotated. Existing remote clients were disconnected.";
    event_hub.publish("remote.client.disconnected", {{"reason", "token_rotated"}});
    log_store.append("warning", "Remote access token rotated");
    send_data(response, body);
  });

  server.Get("/api/remote/clients", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(remote_state.mutex);
    prune_expired_clients(remote_state);

    auto clients = nlohmann::json::array();
    for (const auto& client : remote_state.clients) {
      clients.push_back(client_to_json(client));
    }

    send_data(response, {{"clients", clients}});
  });

  server.Post("/api/remote/disconnect-all", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(remote_state.mutex);
    remote_state.clients.clear();
    event_hub.publish("remote.client.disconnected", {{"reason", "disconnect_all"}});
    log_store.append("warning", "All remote clients disconnected");
    send_data(response, {{"clients", nlohmann::json::array()}, {"message", "All remote clients disconnected."}});
  });

  server.Post("/api/remote/auth", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto token = body.value("token", std::string{});
    const auto pin = body.value("pin", std::string{});

    std::scoped_lock lock(remote_state.mutex);
    prune_expired_clients(remote_state);

    if (!remote_state.enabled || (token != remote_state.access_token && pin != remote_state.pin)) {
      log_store.append("warning", "Remote client authentication failed from " + request.remote_addr);
      send_error(response, "invalid_remote_credentials", "Remote token or PIN is invalid.", 401);
      return;
    }

    const auto now = Clock::now();
    RemoteClient client{
        random_hex(12),
        request.remote_addr.empty() ? "unknown" : request.remote_addr,
        "read-only",
        random_hex(32),
        now,
        now + std::chrono::minutes(kSessionTimeoutMinutes),
    };

    const auto session_token = client.session_token;
    remote_state.clients.push_back(client);
    event_hub.publish("remote.client.connected", client_to_json(remote_state.clients.back()));
    log_store.append("info", "Remote read-only client connected from " + remote_state.clients.back().address);

    send_data(response, {
                            {"sessionToken", session_token},
                            {"permission", "read-only"},
                            {"expiresAt", to_iso_time(remote_state.clients.back().expires_at)},
                        });
  });

  server.Post("/api/files/open-local", [&](const httplib::Request&, httplib::Response& response) {
    send_error(response, "desktop_host_required", "Local file picker requires the Desktop Host.", 501);
  });

  server.Post("/api/files/upload", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"fileId", random_hex(12)}, {"status", "accepted"}, {"storage", "temp"}}, 202);
  });

  server.Get("/api/files/library", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"files", nlohmann::json::array()}});
  });

  server.Delete(R"(/api/files/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, {{"deleted", request.matches[1].str()}});
  });

  server.Post("/api/images/process", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"job", job_queue.enqueue("image", "Image processing placeholder job.")}}, 202);
  });

  server.Post("/api/images/save", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"resultId", random_hex(12)}, {"status", "saved"}});
  });

  server.Get(R"(/api/images/results/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, {{"resultId", request.matches[1].str()}, {"status", "available"}});
  });

  server.Post("/api/videos/process", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"job", job_queue.enqueue("video", "Video processing placeholder job.")}}, 202);
  });

  server.Post("/api/videos/export", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"job", job_queue.enqueue("video-export", "Video export placeholder job.")}}, 202);
  });

  server.Get(R"(/api/videos/frame/([A-Za-z0-9_-]+)/([0-9]+))", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, {{"jobId", request.matches[1].str()}, {"frameIndex", request.matches[2].str()}});
  });

  server.Post("/api/pipelines/execute", [&](const httplib::Request&, httplib::Response& response) {
    const auto job = job_queue.enqueue("pipeline", "Pipeline execution placeholder job.");
    event_hub.publish("pipeline.node.started", {{"jobId", job["id"]}, {"nodeId", "placeholder"}});
    send_data(response, {{"job", job}}, 202);
  });

  server.Get("/api/pipelines", [&](const httplib::Request&, httplib::Response& response) {
    std::scoped_lock lock(pipelines_mutex);
    auto result = nlohmann::json::array();
    for (const auto& [id, pipeline] : pipelines) {
      result.push_back({{"id", id}, {"document", pipeline.document}, {"updatedAt", to_iso_time(pipeline.updated_at)}});
    }
    send_data(response, {{"pipelines", result}});
  });

  server.Post("/api/pipelines", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    PipelineRecord pipeline{random_hex(12), body, Clock::now()};
    {
      std::scoped_lock lock(pipelines_mutex);
      pipelines[pipeline.id] = pipeline;
    }
    send_data(response, {{"id", pipeline.id}, {"document", pipeline.document}}, 201);
  });

  server.Put(R"(/api/pipelines/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto id = request.matches[1].str();
    const auto body = parse_body(request);
    {
      std::scoped_lock lock(pipelines_mutex);
      pipelines[id] = PipelineRecord{id, body, Clock::now()};
    }
    send_data(response, {{"id", id}, {"document", body}});
  });

  server.Delete(R"(/api/pipelines/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto id = request.matches[1].str();
    {
      std::scoped_lock lock(pipelines_mutex);
      pipelines.erase(id);
    }
    send_data(response, {{"deleted", id}});
  });

  server.Get("/api/jobs", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, job_queue.list());
  });

  server.Get(R"(/api/jobs/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto job = job_queue.get(request.matches[1].str());
    if (!job) {
      send_error(response, "job_not_found", "Job was not found.", 404);
      return;
    }
    send_data(response, *job);
  });

  server.Post(R"(/api/jobs/([A-Za-z0-9_-]+)/cancel)", [&](const httplib::Request& request, httplib::Response& response) {
    if (!job_queue.cancel(request.matches[1].str())) {
      send_error(response, "job_not_cancellable", "Job was not found or cannot be cancelled.", 404);
      return;
    }
    send_data(response, {{"cancelled", request.matches[1].str()}});
  });

  server.Delete(R"(/api/jobs/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    if (!job_queue.remove(request.matches[1].str())) {
      send_error(response, "job_not_found", "Job was not found.", 404);
      return;
    }
    send_data(response, {{"deleted", request.matches[1].str()}});
  });

  server.Get("/api/logs/recent", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, log_store.recent());
  });

  server.Get("/api/logs/download", [&](const httplib::Request&, httplib::Response& response) {
    set_cors(response);
    response.set_header("Content-Type", "text/plain");
    response.set_header("Content-Disposition", "attachment; filename=backend.log");
    std::ifstream log_file("logs/backend.log", std::ios::binary);
    std::ostringstream buffer;
    buffer << log_file.rdbuf();
    response.set_content(buffer.str(), "text/plain");
  });

  server.Get("/api/events/recent", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"events", event_hub.recent()}});
  });

  const std::filesystem::path static_root = APP_STATIC_ROOT;
  if (std::filesystem::exists(static_root)) {
    server.set_mount_point("/", static_root.string());
  } else {
    server.Get("/", [](const httplib::Request&, httplib::Response& response) {
      response.set_content(
          "ReactMUIOpenCV backend is running. Build frontend/dist to serve the React UI.",
          "text/plain");
    });
  }

  log_store.append("info", "HTTP backend listening on http://" + host + ":" + std::to_string(port));
  std::cout << "Backend listening on http://" << host << ":" << port << '\n';
  std::cout << "WebSocket listening on ws://" << host << ":" << ws_port << '\n';
  std::cout << "Remote access " << (remote_state.enabled ? "enabled" : "disabled") << '\n';
  std::cout << "LAN URL: http://" << remote_state.selected_ip << ":" << port << '\n';

  if (!server.listen(host, port)) {
    log_store.append("error", "Failed to bind backend on " + host + ":" + std::to_string(port));
    websocket_server.stop();
    ix::uninitNetSystem();
    return EXIT_FAILURE;
  }

  websocket_server.stop();
  ix::uninitNetSystem();
  return EXIT_SUCCESS;
}
