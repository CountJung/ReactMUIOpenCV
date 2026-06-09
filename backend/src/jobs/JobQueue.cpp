#include "JobQueue.h"

#include "../common/Random.h"

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

}  // namespace app
