#include "VideoService.h"

#include "../common/OpenCvUtils.h"
#include "../common/Random.h"
#include "../common/StringUtils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <mutex>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

namespace app {

namespace {

std::string normalize_motion_operation(const std::string& operation) {
  if (operation == "stabilize" || operation == "stabilized") {
    return "stabilize";
  }
  if (operation == "opticalFlow" || operation == "flow") {
    return "opticalFlow";
  }
  return "opticalFlow";
}

cv::Mat draw_optical_flow(const cv::Mat& previous, const cv::Mat& current) {
  cv::Mat previous_gray = to_gray(previous);
  cv::Mat current_gray = to_gray(current);
  cv::Mat flow;
  cv::calcOpticalFlowFarneback(previous_gray, current_gray, flow, 0.5, 3, 15, 3, 5, 1.2, 0);

  cv::Mat visual = current.clone();
  if (visual.channels() == 1) {
    cv::cvtColor(visual, visual, cv::COLOR_GRAY2BGR);
  }

  const int step = std::max(12, std::min(visual.cols, visual.rows) / 24);
  for (int y = step / 2; y < visual.rows; y += step) {
    for (int x = step / 2; x < visual.cols; x += step) {
      const auto vector = flow.at<cv::Point2f>(y, x);
      const cv::Point start(x, y);
      const cv::Point end(
          std::clamp(static_cast<int>(std::round(x + vector.x * 2.0F)), 0, visual.cols - 1),
          std::clamp(static_cast<int>(std::round(y + vector.y * 2.0F)), 0, visual.rows - 1));
      cv::arrowedLine(visual, start, end, cv::Scalar(0, 255, 255), 1, cv::LINE_AA, 0, 0.25);
    }
  }

  return visual;
}

bool estimate_stabilization_transform(
    const cv::Mat& previous,
    const cv::Mat& current,
    cv::Mat& transform,
    int& tracked_features,
    double& average_motion) {
  cv::Mat previous_gray = to_gray(previous);
  cv::Mat current_gray = to_gray(current);
  std::vector<cv::Point2f> previous_points;
  cv::goodFeaturesToTrack(previous_gray, previous_points, 200, 0.01, 30);
  if (previous_points.size() < 8) {
    return false;
  }

  std::vector<cv::Point2f> current_points;
  std::vector<unsigned char> status;
  std::vector<float> errors;
  cv::calcOpticalFlowPyrLK(previous_gray, current_gray, previous_points, current_points, status, errors);

  std::vector<cv::Point2f> matched_previous;
  std::vector<cv::Point2f> matched_current;
  double motion_sum = 0.0;
  double dx_sum = 0.0;
  double dy_sum = 0.0;
  for (std::size_t index = 0; index < status.size(); ++index) {
    if (!status[index]) {
      continue;
    }
    matched_previous.push_back(previous_points[index]);
    matched_current.push_back(current_points[index]);
    const auto delta = current_points[index] - previous_points[index];
    dx_sum += delta.x;
    dy_sum += delta.y;
    motion_sum += std::sqrt((delta.x * delta.x) + (delta.y * delta.y));
  }

  tracked_features = static_cast<int>(matched_previous.size());
  average_motion = tracked_features > 0 ? motion_sum / tracked_features : 0.0;
  if (matched_previous.size() < 8) {
    return false;
  }

  const auto dx = dx_sum / static_cast<double>(tracked_features);
  const auto dy = dy_sum / static_cast<double>(tracked_features);
  transform = (cv::Mat_<double>(2, 3) << 1.0, 0.0, -dx, 0.0, 1.0, -dy);
  return true;
}

cv::Mat stabilize_frame(const cv::Mat& previous, const cv::Mat& current) {
  cv::Mat transform;
  int tracked_features = 0;
  double average_motion = 0.0;
  if (!estimate_stabilization_transform(previous, current, transform, tracked_features, average_motion)) {
    return current.clone();
  }

  cv::Mat output;
  cv::warpAffine(current, output, transform, current.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
  return output;
}

cv::Mat apply_temporal_video_filter(const cv::Mat& previous, const cv::Mat& current, const std::string& filter) {
  if (filter == "opticalFlow") {
    return draw_optical_flow(previous, current);
  }
  if (filter == "stabilize" || filter == "stabilized") {
    return stabilize_frame(previous, current);
  }
  return apply_video_filter(current, filter);
}

}  // namespace

bool is_supported_video_extension(const std::filesystem::path& path) {
  return has_supported_extension(path, {".mp4", ".mov", ".avi", ".mkv", ".wmv", ".m4v"});
}

nlohmann::json video_to_json(const VideoRecord& record) {
  return {
      {"videoId", record.id},
      {"name", record.name},
      {"sourceType", record.source_type},
      {"path", record.path.lexically_normal().string()},
      {"width", record.width},
      {"height", record.height},
      {"fps", record.fps},
      {"frameCount", record.frame_count},
      {"durationSeconds", record.duration_seconds},
      {"createdAt", to_iso_time(record.created_at)},
  };
}

cv::Mat apply_video_filter(const cv::Mat& frame, const std::string& filter) {
  if (filter == "grayscale") {
    return to_gray(frame);
  }

  if (filter == "blur") {
    cv::Mat output;
    cv::GaussianBlur(frame, output, cv::Size(9, 9), 0);
    return output;
  }

  if (filter == "edgeDetect") {
    cv::Mat edges;
    cv::Canny(to_gray(frame), edges, 80, 160);
    return edges;
  }

  if (filter == "threshold") {
    cv::Mat output;
    cv::threshold(to_gray(frame), output, 128, 255, cv::THRESH_BINARY);
    return output;
  }

  if (filter == "opticalFlow" || filter == "stabilize" || filter == "stabilized") {
    return frame.clone();
  }

  return frame.clone();
}

nlohmann::json VideoService::open_local(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    throw std::runtime_error("Video file was not found.");
  }
  if (!is_supported_video_extension(path)) {
    throw std::runtime_error("Unsupported video extension.");
  }

  return add_record(path.filename().string(), "localPath", path);
}

nlohmann::json VideoService::upload(const std::string& filename, const std::string& content) {
  if (!is_supported_video_extension(filename)) {
    throw std::runtime_error("Unsupported video extension.");
  }

  const auto upload_dir = std::filesystem::path("uploads") / "videos";
  std::filesystem::create_directories(upload_dir);
  const auto stored_name =
      random_hex(12) + "_" + sanitize_file_stem(std::filesystem::path(filename).filename().string(), "video");
  const auto output_path = upload_dir / stored_name;

  std::ofstream output(output_path, std::ios::binary);
  output.write(content.data(), static_cast<std::streamsize>(content.size()));
  output.close();

  return add_record(filename.empty() ? "uploaded-video" : filename, "upload", output_path);
}

nlohmann::json VideoService::list() const {
  std::shared_lock lock(mutex_);
  auto videos = nlohmann::json::array();
  for (const auto& [_, record] : videos_) {
    videos.push_back(video_to_json(record));
  }
  return {{"videos", videos}};
}

std::optional<nlohmann::json> VideoService::get(const std::string& id) const {
  const auto record = find_record(id);
  if (!record) {
    return std::nullopt;
  }

  return video_to_json(*record);
}

bool VideoService::remove(const std::string& id) {
  std::unique_lock lock(mutex_);
  return videos_.erase(id) > 0;
}

nlohmann::json VideoService::diagnostics(const std::string& id, int sample_frames) const {
  const auto record = find_record(id);
  if (!record) {
    throw std::runtime_error("Video was not found.");
  }

  sample_frames = std::clamp(sample_frames, 1, std::min(240, std::max(1, record->frame_count)));

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the video for diagnostics.");
  }

  cv::Mat frame;
  int frames_read = 0;
  const auto started = std::chrono::steady_clock::now();
  for (; frames_read < sample_frames; ++frames_read) {
    if (!capture.read(frame) || frame.empty()) {
      break;
    }
  }
  const auto elapsed = std::chrono::steady_clock::now() - started;
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();
  const auto measured_fps = elapsed_ms > 0.0 ? (frames_read * 1000.0) / elapsed_ms : 0.0;

  return {
      {"video", video_to_json(*record)},
      {"sampleFrames", sample_frames},
      {"framesRead", frames_read},
      {"elapsedMs", elapsed_ms},
      {"metadataFps", record->fps},
      {"measuredReadFps", measured_fps},
      {"displayFrameUrl", "/api/videos/frame/" + id + "/0?filter=none"},
      {"writeContainer", "avi"},
      {"writeCodec", "MJPG"},
  };
}

nlohmann::json
VideoService::motion_metrics(const std::string& id, const std::string& operation, int sample_frames) const {
  const auto record = find_record(id);
  if (!record) {
    throw std::runtime_error("Video was not found.");
  }

  sample_frames = std::clamp(sample_frames, 2, std::min(360, std::max(2, record->frame_count)));

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the video for motion analysis.");
  }

  cv::Mat previous;
  cv::Mat current;
  if (!capture.read(previous) || previous.empty()) {
    throw std::runtime_error("OpenCV could not read the first motion-analysis frame.");
  }

  const auto started = std::chrono::steady_clock::now();
  int pairs_read = 0;
  int tracked_sum = 0;
  double flow_sum = 0.0;
  double crop_sum = 0.0;
  const auto normalized_operation = normalize_motion_operation(operation);

  for (int frame_index = 1; frame_index < sample_frames; ++frame_index) {
    if (!capture.read(current) || current.empty()) {
      break;
    }

    cv::Mat transform;
    int tracked_features = 0;
    double average_motion = 0.0;
    estimate_stabilization_transform(previous, current, transform, tracked_features, average_motion);

    tracked_sum += tracked_features;
    flow_sum += average_motion;
    crop_sum += std::clamp((average_motion / std::max(1, std::min(record->width, record->height))) * 100.0, 0.0, 25.0);
    previous = current.clone();
    ++pairs_read;
  }

  const auto elapsed = std::chrono::steady_clock::now() - started;
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();
  const auto safe_pairs = std::max(1, pairs_read);

  return {
      {"video", video_to_json(*record)},
      {"operation", normalized_operation},
      {"sampleFrames", sample_frames},
      {"framesRead", pairs_read + 1},
      {"metadataFps", record->fps},
      {"measuredReadFps", 0.0},
      {"trackedFeatures", tracked_sum / safe_pairs},
      {"averageFlowMagnitude", flow_sum / safe_pairs},
      {"stabilizationCropPercent", crop_sum / safe_pairs},
      {"processingMs", elapsed_ms},
      {"writeContainer", "avi"},
      {"writeCodec", "MJPG"},
      {"previewFrameUrl",
       "/api/videos/frame/" + id + "/" + std::to_string(std::min(1, record->frame_count - 1)) +
           "?filter=" + normalized_operation},
  };
}

nlohmann::json VideoService::track_template(
    const std::string& id,
    int start_frame,
    int end_frame,
    const nlohmann::json& roi,
    const std::function<bool()>& should_cancel,
    const std::function<void(int)>& progress_callback) const {
  const auto record = find_record(id);
  if (!record) {
    throw std::runtime_error("Video was not found.");
  }

  start_frame = std::clamp(start_frame, 0, std::max(0, record->frame_count - 1));
  end_frame = end_frame <= 0 ? std::min(record->frame_count - 1, start_frame + 240) : end_frame;
  end_frame = std::clamp(end_frame, start_frame, std::max(0, record->frame_count - 1));
  end_frame = std::min(end_frame, start_frame + 900);

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the video for object tracking.");
  }

  capture.set(cv::CAP_PROP_POS_FRAMES, start_frame);
  cv::Mat frame;
  if (!capture.read(frame) || frame.empty()) {
    throw std::runtime_error("OpenCV could not read the tracking start frame.");
  }

  cv::Rect current_box = roi_from_json(roi, frame.cols, frame.rows);
  const cv::Mat template_image = to_gray(frame)(current_box).clone();
  const auto started = std::chrono::steady_clock::now();
  auto frames = nlohmann::json::array();
  double score_sum = 0.0;
  int tracked_frames = 0;
  const int total_frames = std::max(1, end_frame - start_frame + 1);

  for (int frame_index = start_frame; frame_index <= end_frame; ++frame_index) {
    if (frame_index != start_frame && (!capture.read(frame) || frame.empty())) {
      break;
    }

    if (should_cancel && should_cancel()) {
      const auto elapsed = std::chrono::steady_clock::now() - started;
      return {
          {"video", video_to_json(*record)},
          {"method", "templateMatch"},
          {"status", "cancelled"},
          {"startFrame", start_frame},
          {"endFrame", frame_index},
          {"sourceRoi", rect_to_json(roi_from_json(roi, record->width, record->height))},
          {"framesTracked", tracked_frames},
          {"averageScore", tracked_frames > 0 ? score_sum / tracked_frames : 0.0},
          {"processingMs", std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count()},
          {"frames", frames},
      };
    }

    const cv::Mat gray = to_gray(frame);
    const cv::Rect search_window = expand_rect(current_box, gray.cols, gray.rows);
    const bool search_is_valid =
        search_window.width >= template_image.cols && search_window.height >= template_image.rows;
    double score = 0.0;
    bool lost = !search_is_valid;

    if (search_is_valid) {
      cv::Mat result;
      cv::matchTemplate(gray(search_window), template_image, result, cv::TM_CCOEFF_NORMED);
      double min_score = 0.0;
      cv::Point min_location;
      cv::Point max_location;
      cv::minMaxLoc(result, &min_score, &score, &min_location, &max_location);
      current_box.x = search_window.x + max_location.x;
      current_box.y = search_window.y + max_location.y;
      lost = score < 0.35;
    }

    frames.push_back({
        {"frameIndex", frame_index},
        {"x", current_box.x},
        {"y", current_box.y},
        {"width", current_box.width},
        {"height", current_box.height},
        {"score", score},
        {"lost", lost},
    });
    score_sum += score;
    ++tracked_frames;

    const int progress = static_cast<int>(((frame_index - start_frame + 1) * 100.0) / total_frames);
    if (progress_callback) {
      progress_callback(std::clamp(progress, 0, 100));
    }
  }

  const auto elapsed = std::chrono::steady_clock::now() - started;
  return {
      {"video", video_to_json(*record)},
      {"method", "templateMatch"},
      {"status", "completed"},
      {"startFrame", start_frame},
      {"endFrame", end_frame},
      {"sourceRoi", rect_to_json(roi_from_json(roi, record->width, record->height))},
      {"framesTracked", tracked_frames},
      {"averageScore", tracked_frames > 0 ? score_sum / tracked_frames : 0.0},
      {"processingMs", std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count()},
      {"frames", frames},
  };
}

std::optional<cv::Mat>
VideoService::read_frame(const std::string& id, int frame_index, const std::string& filter) const {
  const auto record = find_record(id);
  if (!record) {
    return std::nullopt;
  }

  frame_index = std::clamp(frame_index, 0, std::max(0, record->frame_count - 1));

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the video.");
  }

  if (filter == "opticalFlow" || filter == "stabilize" || filter == "stabilized") {
    const auto previous_index = std::max(0, frame_index - 1);
    capture.set(cv::CAP_PROP_POS_FRAMES, previous_index);
    cv::Mat previous;
    cv::Mat current;
    if (!capture.read(previous) || previous.empty()) {
      throw std::runtime_error("OpenCV could not read the previous frame for motion preview.");
    }
    if (previous_index == frame_index) {
      current = previous.clone();
    } else if (!capture.read(current) || current.empty()) {
      throw std::runtime_error("OpenCV could not read the requested motion preview frame.");
    }
    return apply_temporal_video_filter(previous, current, filter);
  }

  capture.set(cv::CAP_PROP_POS_FRAMES, frame_index);
  cv::Mat frame;
  if (!capture.read(frame) || frame.empty()) {
    throw std::runtime_error("OpenCV could not read the requested frame.");
  }

  return apply_video_filter(frame, filter);
}

nlohmann::json VideoService::extract_frame(const std::string& id, int frame_index, const std::string& filter) {
  const auto frame = read_frame(id, frame_index, filter);
  if (!frame) {
    throw std::runtime_error("Video was not found.");
  }

  const auto output_dir = std::filesystem::path("outputs") / "frames";
  std::filesystem::create_directories(output_dir);
  const auto output_path =
      output_dir / (id + "_frame_" + std::to_string(frame_index) + "_" + sanitize_file_stem(filter, "filter") + ".png");

  if (!cv::imwrite(output_path.string(), *frame)) {
    throw std::runtime_error("OpenCV failed to save the extracted frame.");
  }

  return {
      {"videoId", id},
      {"frameIndex", frame_index},
      {"filter", filter},
      {"path", output_path.lexically_normal().string()},
      {"previewUrl", "/api/videos/frame/" + id + "/" + std::to_string(frame_index) + "?filter=" + filter},
  };
}

nlohmann::json VideoService::export_filtered_video(
    const std::string& id,
    const std::string& filter,
    int start_frame,
    int end_frame,
    const std::function<void(int)>& progress_callback) const {
  const auto record = find_record(id);
  if (!record) {
    throw std::runtime_error("Video was not found.");
  }

  start_frame = std::clamp(start_frame, 0, std::max(0, record->frame_count - 1));
  end_frame = end_frame <= 0 ? record->frame_count - 1
                             : std::clamp(end_frame, start_frame, std::max(0, record->frame_count - 1));

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the source video.");
  }

  const auto output_dir = std::filesystem::path("outputs") / "videos";
  std::filesystem::create_directories(output_dir);
  const auto output_path = output_dir / (id + "_" + sanitize_file_stem(filter, "filter") + ".avi");

  const int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  cv::VideoWriter writer(
      output_path.string(), codec, std::max(1.0, record->fps), cv::Size(record->width, record->height), true);
  if (!writer.isOpened()) {
    throw std::runtime_error("OpenCV could not open the output video writer. Check codec availability.");
  }

  capture.set(cv::CAP_PROP_POS_FRAMES, start_frame);
  const int total_frames = std::max(1, end_frame - start_frame + 1);
  cv::Mat previous_frame;

  for (int frame_index = start_frame; frame_index <= end_frame; ++frame_index) {
    cv::Mat frame;
    if (!capture.read(frame) || frame.empty()) {
      break;
    }

    cv::Mat filtered;
    if ((filter == "opticalFlow" || filter == "stabilize" || filter == "stabilized") && !previous_frame.empty()) {
      filtered = apply_temporal_video_filter(previous_frame, frame, filter);
    } else {
      filtered = apply_video_filter(frame, filter);
    }
    previous_frame = frame.clone();
    if (filtered.channels() == 1) {
      cv::cvtColor(filtered, filtered, cv::COLOR_GRAY2BGR);
    }
    if (filtered.size() != cv::Size(record->width, record->height)) {
      cv::resize(filtered, filtered, cv::Size(record->width, record->height));
    }

    writer.write(filtered);

    const int progress = static_cast<int>(((frame_index - start_frame + 1) * 100.0) / total_frames);
    progress_callback(std::clamp(progress, 0, 100));
  }

  return {
      {"videoId", id},
      {"filter", filter},
      {"startFrame", start_frame},
      {"endFrame", end_frame},
      {"path", output_path.lexically_normal().string()},
      {"codec", "MJPG"},
  };
}

nlohmann::json
VideoService::add_record(const std::string& name, const std::string& source_type, const std::filesystem::path& path) {
  cv::VideoCapture capture(path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the video.");
  }

  VideoRecord record;
  record.id = random_hex(12);
  record.name = name;
  record.source_type = source_type;
  record.path = std::filesystem::absolute(path);
  record.width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
  record.height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
  record.fps = capture.get(cv::CAP_PROP_FPS);
  record.frame_count = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT));
  record.duration_seconds = record.fps > 0.0 ? record.frame_count / record.fps : 0.0;
  record.created_at = Clock::now();

  if (record.width <= 0 || record.height <= 0 || record.frame_count <= 0) {
    throw std::runtime_error("OpenCV opened the video but metadata was invalid.");
  }

  std::unique_lock lock(mutex_);
  videos_[record.id] = record;
  return video_to_json(record);
}

std::optional<VideoRecord> VideoService::find_record(const std::string& id) const {
  std::shared_lock lock(mutex_);
  const auto found = videos_.find(id);
  if (found == videos_.end()) {
    return std::nullopt;
  }

  return found->second;
}

}  // namespace app
