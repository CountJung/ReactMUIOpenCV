#include "CalibrationStore.h"

#include "../common/Random.h"
#include "../common/Time.h"

#include <fstream>
#include <utility>

namespace app {

namespace {

constexpr std::size_t kMaxCalibrationRecords = 100;

}  // namespace

CalibrationStore::CalibrationStore(std::filesystem::path storage_path) : storage_path_(std::move(storage_path)) {
  load();
}

nlohmann::json CalibrationStore::list(bool include_storage_path) const {
  std::scoped_lock lock(mutex_);
  nlohmann::json storage = {
      {"kind", "jsonFile"},
      {"description", "Camera calibration artifacts are persisted by the local backend."},
  };
  if (include_storage_path) {
    storage["path"] = std::filesystem::absolute(storage_path_).lexically_normal().string();
  }

  return {{"records", records_}, {"storage", storage}};
}

nlohmann::json CalibrationStore::record(const nlohmann::json& calibration) {
  nlohmann::json item = calibration;
  item["calibrationId"] = random_hex(12);
  item["createdAt"] = to_iso_time(Clock::now());

  std::scoped_lock lock(mutex_);
  records_.insert(records_.begin(), item);
  while (records_.size() > kMaxCalibrationRecords) {
    records_.pop_back();
  }
  save_locked();
  return item;
}

void CalibrationStore::load() {
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
    if (!record.is_object() || !record.contains("calibrationId")) {
      continue;
    }
    records_.push_back(record);
    if (records_.size() >= kMaxCalibrationRecords) {
      break;
    }
  }
}

void CalibrationStore::save_locked() const {
  std::filesystem::create_directories(storage_path_.parent_path());
  const nlohmann::json payload = {
      {"version", 1},
      {"records", records_},
  };

  std::ofstream output(storage_path_, std::ios::binary | std::ios::trunc);
  output << payload.dump(2);
}

}  // namespace app
