#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include <filesystem>
#include <string>

namespace app {

bool is_supported_image_extension(const std::filesystem::path& path);
cv::Mat apply_image_operation(
    const cv::Mat& original, const cv::Mat& source, const std::string& operation, const nlohmann::json& params);

}  // namespace app
