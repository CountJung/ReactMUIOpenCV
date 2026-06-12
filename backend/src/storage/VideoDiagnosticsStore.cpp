#include "VideoDiagnosticsStore.h"

#include "../common/JsonUtils.h"
#include "../common/Random.h"

#include <fstream>
#include <utility>

namespace app {

namespace {

constexpr std::size_t kMaxVideoDiagnosticsRecords = 100;

}  // namespace

VideoDiagnosticsStore::VideoDiagnosticsStore(std::filesystem::path storage_path) : storage_path_(std::move(storage_path)) {
  load();
}

nlohmann::json VideoDiagnosticsStore::list(bool include_storage_path) const {
  std::scoped_lock lock(mutex_);
  nlohmann::json storage = {
      {"kind", "jsonFile"},
      {"description", "Video diagnostics are persisted by the local backend when Measure FPS is executed."},
  };
  if (include_storage_path) {
    storage["path"] = std::filesystem::absolute(storage_path_).lexically_normal().string();
  }

  return {{"records", records_}, {"storage", storage}};
}

nlohmann::json VideoDiagnosticsStore::record(const nlohmann::json& diagnostics) {
  const auto video = diagnostics.value("video", nlohmann::json::object());
  nlohmann::json item = {
      {"diagnosticId", random_hex(12)},
      {"videoId", json_value_or<std::string>(video, "videoId", "")},
      {"videoName", json_value_or<std::string>(video, "name", "video")},
      {"sourceType", json_value_or<std::string>(video, "sourceType", "")},
      {"width", json_value_or<int>(video, "width", 0)},
      {"height", json_value_or<int>(video, "height", 0)},
      {"frameCount", json_value_or<int>(video, "frameCount", 0)},
      {"durationSeconds", json_value_or<double>(video, "durationSeconds", 0.0)},
      {"sampleFrames", json_value_or<int>(diagnostics, "sampleFrames", 0)},
      {"framesRead", json_value_or<int>(diagnostics, "framesRead", 0)},
      {"elapsedMs", json_value_or<double>(diagnostics, "elapsedMs", 0.0)},
      {"metadataFps", json_value_or<double>(diagnostics, "metadataFps", 0.0)},
      {"measuredReadFps", json_value_or<double>(diagnostics, "measuredReadFps", 0.0)},
      {"operation", json_value_or<std::string>(diagnostics, "operation", "readFps")},
      {"trackedFeatures", json_value_or<int>(diagnostics, "trackedFeatures", 0)},
      {"averageFlowMagnitude", json_value_or<double>(diagnostics, "averageFlowMagnitude", 0.0)},
      {"stabilizationCropPercent", json_value_or<double>(diagnostics, "stabilizationCropPercent", 0.0)},
      {"processingMs", json_value_or<double>(diagnostics, "processingMs", json_value_or<double>(diagnostics, "elapsedMs", 0.0))},
      {"writeContainer", json_value_or<std::string>(diagnostics, "writeContainer", "")},
      {"writeCodec", json_value_or<std::string>(diagnostics, "writeCodec", "")},
      {"createdAt", to_iso_time(Clock::now())},
  };

  std::scoped_lock lock(mutex_);
  records_.insert(records_.begin(), item);
  while (records_.size() > kMaxVideoDiagnosticsRecords) {
    records_.pop_back();
  }
  save_locked();
  return item;
}

void VideoDiagnosticsStore::load() {
  std::scoped_lock lock(mutex_);
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
    if (!record.is_object() || !record.contains("diagnosticId")) {
      continue;
    }
    records_.push_back(record);
    if (records_.size() >= kMaxVideoDiagnosticsRecords) {
      break;
    }
  }
}

void VideoDiagnosticsStore::save_locked() const {
  std::filesystem::create_directories(storage_path_.parent_path());
  const nlohmann::json payload = {
      {"version", 1},
      {"records", records_},
  };

  std::ofstream output(storage_path_, std::ios::binary | std::ios::trunc);
  output << payload.dump(2);
}

}  // namespace app
