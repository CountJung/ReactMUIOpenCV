#pragma once

#include "../common/Time.h"

#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <optional>
#include <string>

namespace app {

struct ImageResultRecord {
  std::string id;
  std::string name;
  std::string source_type;
  std::string source_path;
  std::string operation;
  nlohmann::json params;
  cv::Mat original;
  cv::Mat image;
  Clock::time_point created_at;
};

nlohmann::json image_metadata_to_json(const ImageResultRecord& record);

class ImageResultStore {
 public:
  nlohmann::json open_local(const std::filesystem::path& path);
  nlohmann::json upload(const std::string& filename, const std::string& content);
  nlohmann::json process(const std::string& source_id, const std::string& operation, const nlohmann::json& params);
  std::optional<nlohmann::json> get(const std::string& id) const;
  nlohmann::json list() const;
  bool remove(const std::string& id);
  std::optional<cv::Mat> preview(const std::string& id, const std::string& variant) const;
  nlohmann::json save(const std::string& id, const std::string& requested_format) const;

 private:
  nlohmann::json add(
      const std::string& name,
      const std::string& source_type,
      const std::string& source_path,
      const cv::Mat& original,
      const cv::Mat& image,
      const std::string& operation,
      const nlohmann::json& params);

  mutable std::mutex mutex_;
  std::map<std::string, ImageResultRecord> results_;
  std::deque<std::string> order_;
};

}  // namespace app
