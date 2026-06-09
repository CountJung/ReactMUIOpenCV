#pragma once

#include "../common/Time.h"

#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <optional>
#include <string>

namespace app {

struct VideoRecord {
  std::string id;
  std::string name;
  std::string source_type;
  std::filesystem::path path;
  int width = 0;
  int height = 0;
  double fps = 0.0;
  int frame_count = 0;
  double duration_seconds = 0.0;
  Clock::time_point created_at;
};

class VideoService {
 public:
  nlohmann::json open_local(const std::filesystem::path& path);
  nlohmann::json upload(const std::string& filename, const std::string& content);
  nlohmann::json list() const;
  std::optional<nlohmann::json> get(const std::string& id) const;
  std::optional<cv::Mat> read_frame(const std::string& id, int frame_index, const std::string& filter) const;
  nlohmann::json extract_frame(const std::string& id, int frame_index, const std::string& filter);
  nlohmann::json export_filtered_video(
      const std::string& id,
      const std::string& filter,
      int start_frame,
      int end_frame,
      const std::function<void(int)>& progress_callback) const;

 private:
  nlohmann::json add_record(const std::string& name, const std::string& source_type, const std::filesystem::path& path);
  std::optional<VideoRecord> find_record(const std::string& id) const;

  mutable std::mutex mutex_;
  std::map<std::string, VideoRecord> videos_;
};

nlohmann::json video_to_json(const VideoRecord& record);
cv::Mat apply_video_filter(const cv::Mat& frame, const std::string& filter);
bool is_supported_video_extension(const std::filesystem::path& path);

}  // namespace app
