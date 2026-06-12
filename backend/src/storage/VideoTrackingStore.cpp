#include "VideoTrackingStore.h"

#include "../common/JsonUtils.h"
#include "../common/Random.h"
#include "../common/Time.h"

#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace app {

namespace {

constexpr std::size_t kMaxVideoTrackingRecords = 60;

}  // namespace

VideoTrackingStore::VideoTrackingStore(std::filesystem::path storage_path)
    : storage_path_(std::move(storage_path)) {
  load();
}

nlohmann::json VideoTrackingStore::list(bool include_storage_path) const {
  std::shared_lock lock(mutex_);
  nlohmann::json storage = {
      {"kind", "jsonFile"},
      {"description", "Object tracking metadata is persisted by the local backend when Track ROI is executed."},
  };
  if (include_storage_path) {
    storage["path"] = std::filesystem::absolute(storage_path_).lexically_normal().string();
  }

  return {{"records", records_}, {"storage", storage}};
}

nlohmann::json VideoTrackingStore::record(const nlohmann::json& tracking) {
  const auto video = tracking.value("video", nlohmann::json::object());
  const auto frames = tracking.value("frames", nlohmann::json::array());
  nlohmann::json item = {
      {"trackingId", random_hex(12)},
      {"videoId", json_value_or<std::string>(video, "videoId", "")},
      {"videoName", json_value_or<std::string>(video, "name", "video")},
      {"width", json_value_or<int>(video, "width", 0)},
      {"height", json_value_or<int>(video, "height", 0)},
      {"frameCount", json_value_or<int>(video, "frameCount", 0)},
      {"method", json_value_or<std::string>(tracking, "method", "templateMatch")},
      {"status", json_value_or<std::string>(tracking, "status", "completed")},
      {"startFrame", json_value_or<int>(tracking, "startFrame", 0)},
      {"endFrame", json_value_or<int>(tracking, "endFrame", 0)},
      {"framesTracked", json_value_or<int>(tracking, "framesTracked", 0)},
      {"averageScore", json_value_or<double>(tracking, "averageScore", 0.0)},
      {"processingMs", json_value_or<double>(tracking, "processingMs", 0.0)},
      {"sourceRoi", tracking.value("sourceRoi", nlohmann::json::object())},
      {"frames", frames.is_array() ? frames : nlohmann::json::array()},
      {"createdAt", to_iso_time(Clock::now())},
  };

  std::unique_lock lock(mutex_);
  records_.insert(records_.begin(), item);
  while (records_.size() > kMaxVideoTrackingRecords) {
    records_.pop_back();
  }
  save_locked();
  return item;
}

void VideoTrackingStore::load() {
  std::unique_lock lock(mutex_);
  records_.clear();

  std::ifstream input(storage_path_);
  if (!input) {
    return;
  }

  const auto payload = nlohmann::json::parse(input, nullptr, false);
  if (!payload.is_object()) {
    return;
  }

  const auto records = payload.value("records", nlohmann::json::array());
  if (!records.is_array()) {
    return;
  }

  for (const auto& record : records) {
    if (!record.is_object() || !record.contains("trackingId")) {
      continue;
    }
    records_.push_back(record);
    if (records_.size() >= kMaxVideoTrackingRecords) {
      break;
    }
  }
}

void VideoTrackingStore::save_locked() const {
  std::filesystem::create_directories(storage_path_.parent_path());
  const nlohmann::json payload = {
      {"version", 1},
      {"records", records_},
  };

  std::ofstream output(storage_path_, std::ios::binary | std::ios::trunc);
  output << payload.dump(2);
}

}  // namespace app
