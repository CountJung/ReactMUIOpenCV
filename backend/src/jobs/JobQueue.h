#pragma once

#include "../common/Time.h"
#include "../logging/LogStore.h"
#include "../server/EventHub.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <queue>
#include <string>
#include <thread>

namespace app {

struct JobRecord {
  std::string id;
  std::string type;
  std::string status;
  int progress = 0;
  std::string message;
  Clock::time_point created_at;
  Clock::time_point updated_at;
};

nlohmann::json job_to_json(const JobRecord& job);

class JobQueue {
public:
  JobQueue(EventHub& event_hub, LogStore& log_store);
  ~JobQueue();

  JobQueue(const JobQueue&) = delete;
  JobQueue& operator=(const JobQueue&) = delete;

  nlohmann::json enqueue(const std::string& type, const std::string& message);
  nlohmann::json create_manual(const std::string& type, const std::string& message);
  std::optional<nlohmann::json> start(const std::string& id, const std::string& message);
  std::optional<nlohmann::json> progress(const std::string& id, int progress, const std::string& message);
  std::optional<nlohmann::json> complete(const std::string& id, const std::string& message);
  std::optional<nlohmann::json> fail(const std::string& id, const std::string& message);
  nlohmann::json list() const;
  std::optional<nlohmann::json> get(const std::string& id) const;
  bool is_cancelled(const std::string& id) const;
  bool cancel(const std::string& id);
  bool remove(const std::string& id);

private:
  void run();
  nlohmann::json create_record(const std::string& type, const std::string& message, bool enqueue);
  std::optional<nlohmann::json> update_record(
      const std::string& id,
      const std::string& status,
      int progress,
      const std::string& message,
      const std::string& event_type);

  EventHub& event_hub_;
  LogStore& log_store_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::map<std::string, JobRecord> jobs_;
  std::queue<std::string> pending_;
  std::thread worker_;
  bool stopping_ = false;
};

}  // namespace app
