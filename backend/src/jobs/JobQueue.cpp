#include "JobQueue.h"

#include "../common/Random.h"

#include <algorithm>

namespace app {

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

JobQueue::JobQueue(EventHub& event_hub, LogStore& log_store) : event_hub_(event_hub), log_store_(log_store) {
  worker_ = std::thread([this] { run(); });
}

JobQueue::~JobQueue() {
  {
    std::scoped_lock lock(mutex_);
    stopping_ = true;
  }
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

nlohmann::json JobQueue::enqueue(const std::string& type, const std::string& message) {
  return create_record(type, message, true);
}

nlohmann::json JobQueue::create_manual(const std::string& type, const std::string& message) {
  return create_record(type, message, false);
}

nlohmann::json JobQueue::create_record(const std::string& type, const std::string& message, bool enqueue_job) {
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
    if (enqueue_job) {
      pending_.push(job.id);
    }
  }

  log_store_.append("info", "Queued " + type + " job " + job.id);
  if (enqueue_job) {
    cv_.notify_one();
  }
  return job_to_json(job);
}

std::optional<nlohmann::json> JobQueue::start(const std::string& id, const std::string& message) {
  return update_record(id, "running", 0, message, "job.started");
}

std::optional<nlohmann::json> JobQueue::progress(const std::string& id, int progress, const std::string& message) {
  return update_record(id, "running", progress, message, "job.progress");
}

std::optional<nlohmann::json> JobQueue::complete(const std::string& id, const std::string& message) {
  auto job = update_record(id, "completed", 100, message, "job.completed");
  if (job) {
    log_store_.append("info", "Completed pipeline job " + id);
  }
  return job;
}

std::optional<nlohmann::json> JobQueue::fail(const std::string& id, const std::string& message) {
  auto job = update_record(id, "failed", 100, message, "job.failed");
  if (job) {
    log_store_.append("error", "Failed pipeline job " + id + ": " + message);
  }
  return job;
}

std::optional<nlohmann::json> JobQueue::update_record(
    const std::string& id,
    const std::string& status,
    int progress,
    const std::string& message,
    const std::string& event_type) {
  nlohmann::json payload;
  {
    std::scoped_lock lock(mutex_);
    auto found = jobs_.find(id);
    if (found == jobs_.end()) {
      return std::nullopt;
    }

    auto& job = found->second;
    if (job.status == "cancelled" && (status == "running" || status == "completed")) {
      return job_to_json(job);
    }

    job.status = status;
    job.progress = std::clamp(progress, 0, 100);
    job.message = message;
    job.updated_at = Clock::now();
    payload = job_to_json(job);
  }

  event_hub_.publish(event_type, payload);
  return payload;
}

nlohmann::json JobQueue::list() const {
  std::scoped_lock lock(mutex_);
  auto jobs = nlohmann::json::array();
  for (const auto& [_, job] : jobs_) {
    jobs.push_back(job_to_json(job));
  }
  return {{"jobs", jobs}};
}

std::optional<nlohmann::json> JobQueue::get(const std::string& id) const {
  std::scoped_lock lock(mutex_);
  const auto found = jobs_.find(id);
  if (found == jobs_.end()) {
    return std::nullopt;
  }

  return job_to_json(found->second);
}

bool JobQueue::is_cancelled(const std::string& id) const {
  std::scoped_lock lock(mutex_);
  const auto found = jobs_.find(id);
  return found != jobs_.end() && found->second.status == "cancelled";
}

bool JobQueue::cancel(const std::string& id) {
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

bool JobQueue::remove(const std::string& id) {
  std::scoped_lock lock(mutex_);
  auto found = jobs_.find(id);
  if (found == jobs_.end() || found->second.status == "running") {
    return false;
  }

  return jobs_.erase(id) > 0;
}

void JobQueue::run() {
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

      auto found = jobs_.find(job_id);
      if (found == jobs_.end()) {
        continue;
      }

      auto& job = found->second;
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
      auto found = jobs_.find(job_id);
      if (found == jobs_.end()) {
        break;
      }

      auto& job = found->second;
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

}  // namespace app
