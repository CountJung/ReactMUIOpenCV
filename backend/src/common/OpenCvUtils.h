#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

namespace app {

cv::Mat to_bgr(const cv::Mat& input);
cv::Mat to_gray(const cv::Mat& input);
cv::Mat fit_cover(const cv::Mat& input, int width, int height);
cv::Rect roi_from_json(const nlohmann::json& roi, int frame_width, int frame_height, int min_size = 4);
cv::Rect expand_rect(const cv::Rect& box, int frame_width, int frame_height, int min_padding = 24);
nlohmann::json rect_to_json(const cv::Rect& box);

}  // namespace app
