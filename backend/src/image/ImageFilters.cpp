#include "ImageFilters.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <vector>

namespace app {

std::string lowercase_copy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

bool is_supported_image_extension(const std::filesystem::path& path) {
  const auto extension = lowercase_copy(path.extension().string());
  return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" ||
         extension == ".webp" || extension == ".tif" || extension == ".tiff";
}

std::string sanitize_file_stem(std::string value) {
  for (auto& character : value) {
    const auto byte = static_cast<unsigned char>(character);
    if (!std::isalnum(byte) && character != '-' && character != '_') {
      character = '_';
    }
  }

  return value.empty() ? "image" : value;
}

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

namespace {

int json_int(const nlohmann::json& params, const std::string& key, int fallback) {
  if (!params.contains(key) || !params.at(key).is_number_integer()) {
    return fallback;
  }

  return params.at(key).get<int>();
}

double json_double(const nlohmann::json& params, const std::string& key, double fallback) {
  if (!params.contains(key) || !params.at(key).is_number()) {
    return fallback;
  }

  return params.at(key).get<double>();
}

int odd_kernel(int value) {
  value = std::clamp(value, 1, 99);
  return value % 2 == 0 ? value + 1 : value;
}

cv::Mat apply_histogram_preview(const cv::Mat& source) {
  const int width = 512;
  const int height = 320;
  cv::Mat canvas(height, width, CV_8UC3, cv::Scalar(248, 250, 252));

  std::vector<cv::Mat> channels;
  cv::split(to_bgr(source), channels);
  const std::array<cv::Scalar, 3> colors = {
      cv::Scalar(37, 99, 235),
      cv::Scalar(15, 118, 110),
      cv::Scalar(194, 65, 12),
  };

  for (std::size_t index = 0; index < channels.size(); ++index) {
    cv::Mat histogram;
    const int hist_size = 256;
    const float range[] = {0, 256};
    const float* ranges[] = {range};
    cv::calcHist(&channels[index], 1, nullptr, cv::Mat(), histogram, 1, &hist_size, ranges);
    cv::normalize(histogram, histogram, 0, height - 24, cv::NORM_MINMAX);

    const int bin_width = cvRound(static_cast<double>(width) / hist_size);
    for (int i = 1; i < hist_size; ++i) {
      cv::line(canvas,
               cv::Point(bin_width * (i - 1), height - cvRound(histogram.at<float>(i - 1)) - 12),
               cv::Point(bin_width * i, height - cvRound(histogram.at<float>(i)) - 12),
               colors[index],
               2);
    }
  }

  return canvas;
}

}  // namespace

cv::Mat apply_image_operation(
    const cv::Mat& original,
    const cv::Mat& source,
    const std::string& operation,
    const nlohmann::json& params) {
  if (source.empty()) {
    throw std::runtime_error("Source image is empty.");
  }

  if (operation == "open") {
    return source.clone();
  }

  if (operation == "resize") {
    const int width = std::clamp(json_int(params, "width", source.cols), 1, 8192);
    const int height = std::clamp(json_int(params, "height", source.rows), 1, 8192);
    cv::Mat output;
    cv::resize(source, output, cv::Size(width, height), 0, 0, cv::INTER_AREA);
    return output;
  }

  if (operation == "crop") {
    const int x = std::clamp(json_int(params, "x", 0), 0, std::max(0, source.cols - 1));
    const int y = std::clamp(json_int(params, "y", 0), 0, std::max(0, source.rows - 1));
    const int width = std::clamp(json_int(params, "width", source.cols - x), 1, source.cols - x);
    const int height = std::clamp(json_int(params, "height", source.rows - y), 1, source.rows - y);
    return source(cv::Rect(x, y, width, height)).clone();
  }

  if (operation == "rotate") {
    const int angle = ((json_int(params, "angle", 90) % 360) + 360) % 360;
    cv::Mat output;
    if (angle == 90) {
      cv::rotate(source, output, cv::ROTATE_90_CLOCKWISE);
    } else if (angle == 180) {
      cv::rotate(source, output, cv::ROTATE_180);
    } else if (angle == 270) {
      cv::rotate(source, output, cv::ROTATE_90_COUNTERCLOCKWISE);
    } else {
      output = source.clone();
    }
    return output;
  }

  if (operation == "flip") {
    const auto direction = params.value("direction", std::string{"horizontal"});
    const int flip_code = direction == "vertical" ? 0 : direction == "both" ? -1 : 1;
    cv::Mat output;
    cv::flip(source, output, flip_code);
    return output;
  }

  if (operation == "grayscale") {
    return to_gray(source);
  }

  if (operation == "blur") {
    const int kernel = odd_kernel(json_int(params, "kernel", 5));
    cv::Mat output;
    cv::blur(source, output, cv::Size(kernel, kernel));
    return output;
  }

  if (operation == "gaussianBlur") {
    const int kernel = odd_kernel(json_int(params, "kernel", 7));
    cv::Mat output;
    cv::GaussianBlur(source, output, cv::Size(kernel, kernel), 0);
    return output;
  }

  if (operation == "sharpen") {
    cv::Mat output;
    const double strength = std::clamp(json_double(params, "strength", 1.0), 0.2, 4.0);
    const cv::Mat kernel = (cv::Mat_<double>(3, 3) << 0,
                            -strength,
                            0,
                            -strength,
                            1 + 4 * strength,
                            -strength,
                            0,
                            -strength,
                            0);
    cv::filter2D(source, output, source.depth(), kernel);
    return output;
  }

  if (operation == "threshold") {
    cv::Mat output;
    cv::threshold(to_gray(source), output, std::clamp(json_double(params, "threshold", 128.0), 0.0, 255.0), 255, cv::THRESH_BINARY);
    return output;
  }

  if (operation == "edgeDetect") {
    cv::Mat output;
    cv::Canny(to_gray(source), output, std::clamp(json_double(params, "low", 80.0), 0.0, 255.0), std::clamp(json_double(params, "high", 160.0), 0.0, 255.0));
    return output;
  }

  if (operation == "contourDetect") {
    cv::Mat edges;
    cv::Canny(to_gray(source), edges, std::clamp(json_double(params, "low", 80.0), 0.0, 255.0), std::clamp(json_double(params, "high", 160.0), 0.0, 255.0));
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::Mat output = to_bgr(source);
    cv::drawContours(output, contours, -1, cv::Scalar(15, 118, 110), 2);
    return output;
  }

  if (operation == "histogram") {
    return apply_histogram_preview(source);
  }

  if (operation == "colorConvert") {
    const auto target = params.value("target", std::string{"hsv"});
    cv::Mat converted;
    if (target == "gray") {
      return to_gray(source);
    }
    if (target == "lab") {
      cv::cvtColor(to_bgr(source), converted, cv::COLOR_BGR2Lab);
    } else {
      cv::cvtColor(to_bgr(source), converted, cv::COLOR_BGR2HSV);
    }
    return converted;
  }

  if (operation == "compare") {
    cv::Mat baseline = to_bgr(original);
    cv::Mat candidate = to_bgr(source);
    if (baseline.size() != candidate.size()) {
      cv::resize(baseline, baseline, candidate.size(), 0, 0, cv::INTER_AREA);
    }
    cv::Mat diff;
    cv::absdiff(baseline, candidate, diff);
    return diff;
  }

  throw std::runtime_error("Unsupported image operation: " + operation);
}

}  // namespace app
