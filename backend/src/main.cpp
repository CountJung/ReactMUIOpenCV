#include "common/Constants.h"
#include "image/ImageResultStore.h"
#include "jobs/JobQueue.h"
#include "logging/LogStore.h"
#include "security/RemoteAccessManager.h"
#include "server/ApiServer.h"
#include "server/EventHub.h"
#include "server/NetworkInfo.h"
#include "server/WebSocketGateway.h"
#include "storage/PipelineStore.h"
#include "storage/SettingsStore.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

bool has_arg(int argc, char* argv[], const std::string& value) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == value) {
      return true;
    }
  }

  return false;
}

std::optional<std::string> arg_value(int argc, char* argv[], const std::string& name) {
  const std::string prefix = name + "=";
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == name && i + 1 < argc) {
      return std::string(argv[i + 1]);
    }
    if (arg.rfind(prefix, 0) == 0) {
      return arg.substr(prefix.size());
    }
  }

  return std::nullopt;
}

std::filesystem::path executable_dir(char* argv0) {
  return std::filesystem::absolute(std::filesystem::path(argv0)).parent_path();
}

std::filesystem::path resolve_static_root(int argc, char* argv[]) {
  if (const auto override_path = arg_value(argc, argv, "--static-root")) {
    return std::filesystem::absolute(*override_path);
  }

  const auto exe_dir = executable_dir(argv[0]);
  const std::vector<std::filesystem::path> candidates = {
      exe_dir / "frontend" / "dist",
      exe_dir / "dist",
      std::filesystem::current_path() / "frontend" / "dist",
      std::filesystem::path(APP_STATIC_ROOT),
  };

  for (const auto& candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }

  return candidates.front();
}

}  // namespace

int main(int argc, char* argv[]) {
  app::configure_file_logging();

  const bool lan_mode = has_arg(argc, argv, "--lan");
  const std::string host = lan_mode ? "0.0.0.0" : "127.0.0.1";
  const auto lan_addresses = app::get_lan_ipv4_addresses();
  const std::string selected_ip = lan_addresses.empty() ? "127.0.0.1" : lan_addresses.front();
  const auto static_root = resolve_static_root(argc, argv);

  app::EventHub event_hub;
  app::LogStore log_store(event_hub);
  app::JobQueue job_queue(event_hub, log_store);
  app::SettingsStore settings_store;
  app::RemoteAccessManager remote_access(lan_mode, host, selected_ip);
  app::ImageResultStore image_store;
  app::PipelineStore pipeline_store;

  app::WebSocketGateway websocket_gateway(host, app::kDefaultWsPort, event_hub, log_store);
  websocket_gateway.start();

  app::ApiServer api_server(
      host,
      app::kDefaultPort,
      app::kDefaultWsPort,
      lan_addresses,
      event_hub,
      log_store,
      job_queue,
      settings_store,
      remote_access,
      image_store,
      pipeline_store,
      static_root);

  std::cout << "Backend listening on http://" << host << ":" << app::kDefaultPort << '\n';
  std::cout << "WebSocket listening on ws://" << host << ":" << app::kDefaultWsPort << '\n';
  std::cout << "Remote access " << (remote_access.enabled() ? "enabled" : "disabled") << '\n';
  std::cout << "LAN URL: http://" << selected_ip << ":" << app::kDefaultPort << '\n';
  std::cout << "Static root: " << static_root.string() << '\n';

  if (!api_server.listen()) {
    log_store.append("error", "Failed to bind backend on " + host + ":" + std::to_string(app::kDefaultPort));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
