#include "PerformanceBenchmarkStore.h"

#include "../common/JsonUtils.h"
#include "../common/Random.h"
#include "../common/Time.h"

#include <algorithm>
#include <fstream>
#include <utility>

namespace app {

namespace {

constexpr std::size_t kMaxPerformanceBenchmarkRecords = 200;

}  // namespace

PerformanceBenchmarkStore::PerformanceBenchmarkStore(std::filesystem::path storage_path)
    : storage_path_(std::move(storage_path)) {
  load();
}

nlohmann::json PerformanceBenchmarkStore::list(bool include_storage_path) const {
  std::scoped_lock lock(mutex_);
  nlohmann::json storage = {
      {"kind", "jsonFile"},
      {"description", "OpenCV pixel-access benchmark samples are persisted by the local backend."},
  };
  if (include_storage_path) {
    storage["path"] = std::filesystem::absolute(storage_path_).lexically_normal().string();
  }

  return {{"records", records_}, {"storage", storage}};
}

nlohmann::json PerformanceBenchmarkStore::record(const nlohmann::json& benchmark) {
  nlohmann::json item = benchmark.is_object() ? benchmark : nlohmann::json::object();
  item["benchmarkId"] = random_hex(12);
  item["imageResultId"] = json_value_or<std::string>(benchmark, "imageResultId", "");
  item["imageName"] = json_value_or<std::string>(benchmark, "imageName", "image");
  item["createdAt"] = to_iso_time(Clock::now());

  std::scoped_lock lock(mutex_);
  records_.insert(records_.begin(), item);
  while (records_.size() > kMaxPerformanceBenchmarkRecords) {
    records_.pop_back();
  }
  save_locked();
  return item;
}

bool PerformanceBenchmarkStore::remove(const std::string& benchmark_id) {
  std::scoped_lock lock(mutex_);
  const auto old_size = records_.size();
  std::erase_if(records_, [&](const auto& record) {
    return record.is_object() && record.value("benchmarkId", std::string{}) == benchmark_id;
  });
  if (records_.size() == old_size) {
    return false;
  }
  save_locked();
  return true;
}

int PerformanceBenchmarkStore::clear() {
  std::scoped_lock lock(mutex_);
  const auto count = static_cast<int>(records_.size());
  records_.clear();
  save_locked();
  return count;
}

void PerformanceBenchmarkStore::load() {
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
    if (!record.is_object() || !record.contains("benchmarkId")) {
      continue;
    }
    records_.push_back(record);
    if (records_.size() >= kMaxPerformanceBenchmarkRecords) {
      break;
    }
  }
}

void PerformanceBenchmarkStore::save_locked() const {
  std::filesystem::create_directories(storage_path_.parent_path());
  const nlohmann::json payload = {
      {"version", 1},
      {"records", records_},
  };

  std::ofstream output(storage_path_, std::ios::binary | std::ios::trunc);
  output << payload.dump(2);
}

}  // namespace app
