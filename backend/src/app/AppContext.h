#pragma once

#include "../image/ImageResultStore.h"
#include "../jobs/JobQueue.h"
#include "../logging/LogStore.h"
#include "../security/RemoteAccessManager.h"
#include "../server/ApiServer.h"
#include "../server/EventHub.h"
#include "../server/WebSocketGateway.h"
#include "../storage/PipelineStore.h"
#include "../storage/SettingsStore.h"
#include "../storage/VideoDiagnosticsStore.h"
#include "../storage/VideoTrackingStore.h"
#include "../video/VideoService.h"
#include "../vision/PipelineExecutor.h"

#include <filesystem>
#include <string>
#include <vector>

namespace app {

struct AppRuntimeConfig {
  bool lan_mode = false;
  std::string host = "127.0.0.1";
  std::string selected_ip = "127.0.0.1";
  int http_port = 18730;
  int ws_port = 18731;
  std::vector<std::string> lan_addresses;
  std::filesystem::path static_root;
  std::filesystem::path data_dir;
};

AppRuntimeConfig build_app_runtime_config(int argc, char* argv[]);

class AppContext {
public:
  explicit AppContext(AppRuntimeConfig config);
  ~AppContext() = default;
  // 이런 행위를 막음
  AppContext(const AppContext&) = delete;
  AppContext& operator=(const AppContext&) = delete;
  AppContext(AppContext&&) = delete;
  AppContext& operator=(AppContext&&) = delete;

  int run();

private:
  AppRuntimeConfig config_;
  EventHub event_hub_;
  LogStore log_store_;
  JobQueue job_queue_;
  SettingsStore settings_store_;
  RemoteAccessManager remote_access_;
  ImageResultStore image_store_;
  VideoService video_service_;
  PipelineStore pipeline_store_;
  VideoDiagnosticsStore video_diagnostics_store_;
  VideoTrackingStore video_tracking_store_;
  PipelineExecutor pipeline_executor_;
  WebSocketGateway websocket_gateway_;
  ApiServer api_server_;
};

}  // namespace app
