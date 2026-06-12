#include "OpenCvUtils.h"

#include "JsonUtils.h"

#include <algorithm>
#include <opencv2/imgproc.hpp>
#include <stdexcept>

namespace app {

cv::Mat to_bgr(const cv::Mat& input) {
  cv::Mat output;
  if (input.channels() == 1) {
    cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
  } else if (input.channels() == 4) {
    cv::cvtColor(input, output, cv::COLOR_BGRA2BGR);
  } else {
    output = input.clone();
  }

  return output;
}

cv::Mat to_gray(const cv::Mat& input) {
  cv::Mat output;
  if (input.channels() == 1) {
    output = input.clone();
  } else if (input.channels() == 4) {
    cv::cvtColor(input, output, cv::COLOR_BGRA2GRAY);
  } else {
    cv::cvtColor(input, output, cv::COLOR_BGR2GRAY);
  }

  return output;
}

cv::Rect roi_from_json(const nlohmann::json& roi, int frame_width, int frame_height, int min_size) {
  const int x = json_value_or<int>(roi, "x", 0);
  const int y = json_value_or<int>(roi, "y", 0);
  const int width = json_value_or<int>(roi, "width", std::max(8, frame_width / 5));
  const int height = json_value_or<int>(roi, "height", std::max(8, frame_height / 5));

  if (width <= 0 || height <= 0) {
    throw std::runtime_error("ROI width and height must be positive.");
  }

  const int clamped_x = std::clamp(x, 0, std::max(0, frame_width - 1));
  const int clamped_y = std::clamp(y, 0, std::max(0, frame_height - 1));
  const int clamped_width = std::clamp(width, 1, frame_width - clamped_x);
  const int clamped_height = std::clamp(height, 1, frame_height - clamped_y);
  if (clamped_width < min_size || clamped_height < min_size) {
    throw std::runtime_error("ROI is too small after clamping to the frame.");
  }

  return {clamped_x, clamped_y, clamped_width, clamped_height};
}

cv::Rect expand_rect(const cv::Rect& box, int frame_width, int frame_height, int min_padding) {
  const int padding = std::max({min_padding, box.width, box.height});
  const int x = std::max(0, box.x - padding);
  const int y = std::max(0, box.y - padding);
  const int right = std::min(frame_width, box.x + box.width + padding);
  const int bottom = std::min(frame_height, box.y + box.height + padding);
  return {x, y, std::max(1, right - x), std::max(1, bottom - y)};
}

nlohmann::json rect_to_json(const cv::Rect& box) {
  return {{"x", box.x}, {"y", box.y}, {"width", box.width}, {"height", box.height}};
}

}  // namespace app
