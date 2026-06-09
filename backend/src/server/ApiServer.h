#pragma once

#include "../image/ImageResultStore.h"
#include "../jobs/JobQueue.h"
#include "../logging/LogStore.h"
#include "../security/RemoteAccessManager.h"
#include "../storage/PipelineStore.h"
#include "../storage/SettingsStore.h"
#include "EventHub.h"

#include <httplib.h>

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
      PipelineStore& pipeline_store);

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
  PipelineStore& pipeline_store_;
  httplib::Server server_;
};

}  // namespace app
