#include "AppContext.h"

#include "../common/Constants.h"
#include "../server/NetworkInfo.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace app {

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

std::filesystem::path resolve_data_dir(char* argv0) {
  const auto cwd_data = std::filesystem::current_path() / "data";
  if (std::filesystem::exists(std::filesystem::current_path() / "frontend") ||
      std::filesystem::exists(std::filesystem::current_path() / "backend")) {
    return cwd_data;
  }

  return executable_dir(argv0) / "data";
}

}  // namespace

AppRuntimeConfig build_app_runtime_config(int argc, char* argv[]) {
  AppRuntimeConfig config;
  config.lan_mode = has_arg(argc, argv, "--lan");
  config.host = config.lan_mode ? "0.0.0.0" : "127.0.0.1";
  config.http_port = kDefaultPort;
  config.ws_port = kDefaultWsPort;
  config.lan_addresses = get_lan_ipv4_addresses();
  config.selected_ip = config.lan_addresses.empty() ? "127.0.0.1" : config.lan_addresses.front();
  config.static_root = resolve_static_root(argc, argv);
  config.data_dir = resolve_data_dir(argv[0]);
  return config;
}

AppContext::AppContext(AppRuntimeConfig config)
    : config_(std::move(config)),
      log_store_(event_hub_),
      job_queue_(event_hub_, log_store_),
      remote_access_(config_.lan_mode, config_.host, config_.selected_ip),
      pipeline_store_(config_.data_dir / "pipelines.json"),
      video_diagnostics_store_(config_.data_dir / "video-diagnostics.json"),
      video_tracking_store_(config_.data_dir / "video-tracking.json"),
      calibration_store_(config_.data_dir / "calibration-results.json"),
      calibration_service_(image_store_, calibration_store_),
      pipeline_executor_(image_store_, video_service_, video_tracking_store_, event_hub_, job_queue_, log_store_),
      websocket_gateway_(config_.host, config_.ws_port, event_hub_, log_store_, remote_access_),
      api_server_(
          config_.host,
          config_.http_port,
          config_.ws_port,
          config_.lan_addresses,
          event_hub_,
          log_store_,
          job_queue_,
          settings_store_,
          remote_access_,
          image_store_,
          video_service_,
          pipeline_store_,
          video_diagnostics_store_,
          video_tracking_store_,
          calibration_store_,
          calibration_service_,
          pipeline_executor_,
          config_.static_root) {}

int AppContext::run() {
  websocket_gateway_.start();

  std::cout << "Backend listening on http://" << config_.host << ":" << config_.http_port << '\n';
  std::cout << "WebSocket listening on ws://" << config_.host << ":" << config_.ws_port << '\n';
  std::cout << "Remote access " << (remote_access_.enabled() ? "enabled" : "disabled") << '\n';
  std::cout << "LAN URL: http://" << config_.selected_ip << ":" << config_.http_port << '\n';
  std::cout << "Static root: " << config_.static_root.string() << '\n';
  std::cout << "Data dir: " << config_.data_dir.string() << '\n';

  if (!api_server_.listen()) {
    log_store_.append("error", "Failed to bind backend on " + config_.host + ":" + std::to_string(config_.http_port));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

}  // namespace app
