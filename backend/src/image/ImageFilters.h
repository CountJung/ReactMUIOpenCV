#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include <filesystem>
#include <string>

namespace app {

std::string lowercase_copy(std::string value);
bool is_supported_image_extension(const std::filesystem::path& path);
std::string sanitize_file_stem(std::string value);
cv::Mat to_bgr(const cv::Mat& input);
cv::Mat to_gray(const cv::Mat& input);
cv::Mat apply_image_operation(
    const cv::Mat& original,
    const cv::Mat& source,
    const std::string& operation,
    const nlohmann::json& params);

}  // namespace app
