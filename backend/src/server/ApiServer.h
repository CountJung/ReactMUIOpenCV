#pragma once

#include "../image/ImageResultStore.h"
#include "../jobs/JobQueue.h"
#include "../logging/LogStore.h"
#include "../security/RemoteAccessManager.h"
#include "../storage/CalibrationStore.h"
#include "../storage/PipelineStore.h"
#include "../storage/SettingsStore.h"
#include "../storage/VideoDiagnosticsStore.h"
#include "../storage/VideoTrackingStore.h"
#include "../video/VideoService.h"
#include "../vision/CalibrationService.h"
#include "../vision/PipelineExecutor.h"
#include "EventHub.h"

#include <httplib.h>

#include <filesystem>
#include <string>
#include <vector>

namespace app {

class ApiServer {
public:
  ApiServer(
      std::string host,
      int port,
      int ws_port,
      std::vector<std::string> lan_addresses,
      EventHub& event_hub,
      LogStore& log_store,
      JobQueue& job_queue,
      SettingsStore& settings_store,
      RemoteAccessManager& remote_access,
      ImageResultStore& image_store,
      VideoService& video_service,
      PipelineStore& pipeline_store,
      VideoDiagnosticsStore& video_diagnostics_store,
      VideoTrackingStore& video_tracking_store,
      CalibrationStore& calibration_store,
      CalibrationService& calibration_service,
      PipelineExecutor& pipeline_executor,
      std::filesystem::path static_root);

  bool listen();

private:
  void register_routes();
  void mount_static_files();

  std::string host_;
  int port_;
  int ws_port_;
  std::vector<std::string> lan_addresses_;
  EventHub& event_hub_;
  LogStore& log_store_;
  JobQueue& job_queue_;
  SettingsStore& settings_store_;
  RemoteAccessManager& remote_access_;
  ImageResultStore& image_store_;
  VideoService& video_service_;
  PipelineStore& pipeline_store_;
  VideoDiagnosticsStore& video_diagnostics_store_;
  VideoTrackingStore& video_tracking_store_;
  CalibrationStore& calibration_store_;
  CalibrationService& calibration_service_;
  PipelineExecutor& pipeline_executor_;
  std::filesystem::path static_root_;
  httplib::Server server_;
};

}  // namespace app
