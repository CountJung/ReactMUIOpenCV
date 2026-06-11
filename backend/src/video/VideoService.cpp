#include "VideoService.h"

#include "../common/Random.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <stdexcept>
#include <vector>

namespace app {

namespace {

std::string lowercase_copy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

std::string sanitize_file_stem(std::string value) {
  for (auto& character : value) {
    const auto byte = static_cast<unsigned char>(character);
    if (!std::isalnum(byte) && character != '-' && character != '_') {
      character = '_';
    }
  }

  return value.empty() ? "video" : value;
}

cv::Mat to_gray(const cv::Mat& frame) {
  cv::Mat gray;
  if (frame.channels() == 1) {
    gray = frame.clone();
  } else {
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
  }
  return gray;
}

}  // namespace

bool is_supported_video_extension(const std::filesystem::path& path) {
  const auto extension = lowercase_copy(path.extension().string());
  return extension == ".mp4" || extension == ".mov" || extension == ".avi" || extension == ".mkv" ||
         extension == ".wmv" || extension == ".m4v";
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
  const auto stored_name = random_hex(12) + "_" + sanitize_file_stem(std::filesystem::path(filename).filename().string());
  const auto output_path = upload_dir / stored_name;

  std::ofstream output(output_path, std::ios::binary);
  output.write(content.data(), static_cast<std::streamsize>(content.size()));
  output.close();

  return add_record(filename.empty() ? "uploaded-video" : filename, "upload", output_path);
}

nlohmann::json VideoService::list() const {
  std::scoped_lock lock(mutex_);
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
  std::scoped_lock lock(mutex_);
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

std::optional<cv::Mat> VideoService::read_frame(const std::string& id, int frame_index, const std::string& filter) const {
  const auto record = find_record(id);
  if (!record) {
    return std::nullopt;
  }

  frame_index = std::clamp(frame_index, 0, std::max(0, record->frame_count - 1));

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the video.");
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
  const auto output_path = output_dir / (id + "_frame_" + std::to_string(frame_index) + "_" + sanitize_file_stem(filter) + ".png");

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
  end_frame = end_frame <= 0 ? record->frame_count - 1 : std::clamp(end_frame, start_frame, std::max(0, record->frame_count - 1));

  cv::VideoCapture capture(record->path.string());
  if (!capture.isOpened()) {
    throw std::runtime_error("OpenCV could not open the source video.");
  }

  const auto output_dir = std::filesystem::path("outputs") / "videos";
  std::filesystem::create_directories(output_dir);
  const auto output_path = output_dir / (id + "_" + sanitize_file_stem(filter) + ".avi");

  const int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  cv::VideoWriter writer(output_path.string(), codec, std::max(1.0, record->fps), cv::Size(record->width, record->height), true);
  if (!writer.isOpened()) {
    throw std::runtime_error("OpenCV could not open the output video writer. Check codec availability.");
  }

  capture.set(cv::CAP_PROP_POS_FRAMES, start_frame);
  const int total_frames = std::max(1, end_frame - start_frame + 1);

  for (int frame_index = start_frame; frame_index <= end_frame; ++frame_index) {
    cv::Mat frame;
    if (!capture.read(frame) || frame.empty()) {
      break;
    }

    auto filtered = apply_video_filter(frame, filter);
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

nlohmann::json VideoService::add_record(const std::string& name, const std::string& source_type, const std::filesystem::path& path) {
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

  std::scoped_lock lock(mutex_);
  videos_[record.id] = record;
  return video_to_json(record);
}

std::optional<VideoRecord> VideoService::find_record(const std::string& id) const {
  std::scoped_lock lock(mutex_);
  const auto found = videos_.find(id);
  if (found == videos_.end()) {
    return std::nullopt;
  }

  return found->second;
}

}  // namespace app
