#include "ImageResultStore.h"

#include "../common/Constants.h"
#include "../common/Random.h"
#include "../common/StringUtils.h"
#include "ImageFilters.h"

#include <algorithm>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>
#include <vector>

namespace app {

nlohmann::json image_metadata_to_json(const ImageResultRecord& record) {
  return {
      {"resultId", record.id},
      {"name", record.name},
      {"sourceType", record.source_type},
      {"sourcePath", record.source_path},
      {"operation", record.operation},
      {"params", record.params},
      {"width", record.image.cols},
      {"height", record.image.rows},
      {"channels", record.image.channels()},
      {"createdAt", to_iso_time(record.created_at)},
      {"previewUrl", "/api/images/results/" + record.id + "/preview?variant=result"},
      {"originalPreviewUrl", "/api/images/results/" + record.id + "/preview?variant=original"},
  };
}

nlohmann::json ImageResultStore::open_local(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    throw std::runtime_error("Image file was not found.");
  }
  if (!is_supported_image_extension(path)) {
    throw std::runtime_error("Unsupported image extension.");
  }

  const auto image = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
  if (image.empty()) {
    throw std::runtime_error("OpenCV could not decode the image.");
  }

  return add(path.filename().string(), "localPath", path.lexically_normal().string(), image, image, "open", nlohmann::json::object());
}

nlohmann::json ImageResultStore::upload(const std::string& filename, const std::string& content) {
  std::vector<unsigned char> bytes(content.begin(), content.end());
  const auto image = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
  if (image.empty()) {
    throw std::runtime_error("OpenCV could not decode the uploaded image.");
  }

  return add(filename.empty() ? "uploaded-image" : filename, "upload", "", image, image, "open", nlohmann::json::object());
}

nlohmann::json ImageResultStore::process(const std::string& source_id, const std::string& operation, const nlohmann::json& params) {
  ImageResultRecord source;
  {
    std::scoped_lock lock(mutex_);
    const auto found = results_.find(source_id);
    if (found == results_.end()) {
      throw std::runtime_error("Image result was not found.");
    }
    source = found->second;
  }

  const auto processed = apply_image_operation(source.original, source.image, operation, params);
  return add(source.name, source.source_type, source.source_path, source.original, processed, operation, params);
}

std::optional<nlohmann::json> ImageResultStore::get(const std::string& id) const {
  std::scoped_lock lock(mutex_);
  const auto found = results_.find(id);
  if (found == results_.end()) {
    return std::nullopt;
  }

  return image_metadata_to_json(found->second);
}

nlohmann::json ImageResultStore::list() const {
  std::scoped_lock lock(mutex_);
  auto results = nlohmann::json::array();
  for (const auto& id : order_) {
    const auto found = results_.find(id);
    if (found != results_.end()) {
      results.push_back(image_metadata_to_json(found->second));
    }
  }
  return {{"results", results}};
}

bool ImageResultStore::remove(const std::string& id) {
  std::scoped_lock lock(mutex_);
  const auto erased = results_.erase(id) > 0;
  if (erased) {
    std::erase(order_, id);
  }
  return erased;
}

std::optional<cv::Mat> ImageResultStore::preview(const std::string& id, const std::string& variant) const {
  std::scoped_lock lock(mutex_);
  const auto found = results_.find(id);
  if (found == results_.end()) {
    return std::nullopt;
  }

  return variant == "original" ? found->second.original.clone() : found->second.image.clone();
}

nlohmann::json ImageResultStore::save(const std::string& id, const std::string& requested_format) const {
  ImageResultRecord record;
  {
    std::scoped_lock lock(mutex_);
    const auto found = results_.find(id);
    if (found == results_.end()) {
      throw std::runtime_error("Image result was not found.");
    }
    record = found->second;
  }

  const auto format = requested_format == "jpg" || requested_format == "jpeg" ? "jpg" : "png";
  const auto output_dir = std::filesystem::path("outputs") / "images";
  std::filesystem::create_directories(output_dir);
  const auto filename = sanitize_file_stem(record.id + "_" + record.operation, "image") + "." + format;
  const auto output_path = output_dir / filename;
  if (!cv::imwrite(output_path.string(), record.image)) {
    throw std::runtime_error("OpenCV failed to save the image.");
  }

  return {{"resultId", id}, {"status", "saved"}, {"path", output_path.lexically_normal().string()}};
}

nlohmann::json ImageResultStore::add(
    const std::string& name,
    const std::string& source_type,
    const std::string& source_path,
    const cv::Mat& original,
    const cv::Mat& image,
    const std::string& operation,
    const nlohmann::json& params) {
  ImageResultRecord record;
  record.id = random_hex(12);
  record.name = name;
  record.source_type = source_type;
  record.source_path = source_path;
  record.operation = operation;
  record.params = params;
  record.original = original.clone();
  record.image = image.clone();
  record.created_at = Clock::now();

  std::scoped_lock lock(mutex_);
  results_[record.id] = record;
  order_.push_front(record.id);
  while (order_.size() > kMaxImageResults) {
    results_.erase(order_.back());
    order_.pop_back();
  }

  return image_metadata_to_json(results_.at(record.id));
}

}  // namespace app
