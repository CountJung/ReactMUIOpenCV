#include "ImageFilters.h"

#include "../common/OpenCvUtils.h"
#include "../common/StringUtils.h"

#include <algorithm>
#include <array>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/video/tracking.hpp>
#include <stdexcept>
#include <vector>

namespace app {

bool is_supported_image_extension(const std::filesystem::path& path) {
  return has_supported_extension(path, {".png", ".jpg", ".jpeg", ".bmp", ".webp", ".tif", ".tiff"});
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
      cv::line(
          canvas,
          cv::Point(bin_width * (i - 1), height - cvRound(histogram.at<float>(i - 1)) - 12),
          cv::Point(bin_width * i, height - cvRound(histogram.at<float>(i)) - 12),
          colors[index],
          2);
    }
  }

  return canvas;
}

cv::Mat apply_feature_alignment(const cv::Mat& reference, const cv::Mat& source, const nlohmann::json& params) {
  if (reference.empty()) {
    throw std::runtime_error("Reference image is empty.");
  }

  const int max_features = std::clamp(json_int(params, "maxFeatures", 500), 50, 5000);
  const double keep_ratio = std::clamp(json_double(params, "keepRatio", 0.18), 0.05, 1.0);

  const auto reference_gray = to_gray(reference);
  const auto source_gray = to_gray(source);
  auto orb = cv::ORB::create(max_features);
  std::vector<cv::KeyPoint> reference_keypoints;
  std::vector<cv::KeyPoint> source_keypoints;
  cv::Mat reference_descriptors;
  cv::Mat source_descriptors;
  orb->detectAndCompute(reference_gray, cv::noArray(), reference_keypoints, reference_descriptors);
  orb->detectAndCompute(source_gray, cv::noArray(), source_keypoints, source_descriptors);

  if (reference_descriptors.empty() || source_descriptors.empty()) {
    throw std::runtime_error("Not enough features were detected for alignment.");
  }

  std::vector<cv::DMatch> matches;
  cv::BFMatcher(cv::NORM_HAMMING, true).match(source_descriptors, reference_descriptors, matches);
  if (matches.size() < 4) {
    throw std::runtime_error("Not enough feature matches were found for alignment.");
  }

  std::sort(matches.begin(), matches.end(), [](const cv::DMatch& left, const cv::DMatch& right) {
    return left.distance < right.distance;
  });
  matches.resize(std::max<std::size_t>(4, static_cast<std::size_t>(matches.size() * keep_ratio)));

  std::vector<cv::Point2f> source_points;
  std::vector<cv::Point2f> reference_points;
  source_points.reserve(matches.size());
  reference_points.reserve(matches.size());
  for (const auto& match : matches) {
    source_points.push_back(source_keypoints[match.queryIdx].pt);
    reference_points.push_back(reference_keypoints[match.trainIdx].pt);
  }

  const auto homography = cv::findHomography(source_points, reference_points, cv::RANSAC);
  if (homography.empty()) {
    throw std::runtime_error("OpenCV could not estimate an alignment transform.");
  }

  cv::Mat output;
  cv::warpPerspective(to_bgr(source), output, homography, reference.size());
  return output;
}

cv::Mat apply_ecc_alignment(const cv::Mat& reference, const cv::Mat& source, const nlohmann::json& params) {
  if (reference.empty()) {
    throw std::runtime_error("Reference image is empty.");
  }

  cv::Mat reference_gray;
  cv::Mat source_gray;
  to_gray(reference).convertTo(reference_gray, CV_32F, 1.0 / 255.0);
  cv::resize(to_gray(source), source_gray, reference.size(), 0, 0, cv::INTER_AREA);
  source_gray.convertTo(source_gray, CV_32F, 1.0 / 255.0);

  cv::Mat warp_matrix = cv::Mat::eye(2, 3, CV_32F);
  const int iterations = std::clamp(json_int(params, "iterations", 80), 10, 1000);
  const double epsilon = std::clamp(json_double(params, "epsilon", 0.0001), 0.000001, 0.01);
  try {
    cv::findTransformECC(
        reference_gray,
        source_gray,
        warp_matrix,
        cv::MOTION_AFFINE,
        cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, iterations, epsilon));
  } catch (const cv::Exception& error) {
    throw std::runtime_error("ECC alignment failed: " + std::string(error.what()));
  }

  cv::Mat resized_source;
  cv::resize(to_bgr(source), resized_source, reference.size(), 0, 0, cv::INTER_AREA);
  cv::Mat output;
  cv::warpAffine(
      resized_source,
      output,
      warp_matrix,
      reference.size(),
      cv::INTER_LINEAR | cv::WARP_INVERSE_MAP,
      cv::BORDER_CONSTANT,
      cv::Scalar::all(0));
  return output;
}

cv::Mat apply_qr_overlay(const cv::Mat& source) {
  cv::Mat output = to_bgr(source);
  cv::QRCodeDetector detector;
  std::vector<cv::Point> points;
  const auto decoded = detector.detectAndDecode(output, points);

  const auto accent = decoded.empty() ? cv::Scalar(37, 99, 235) : cv::Scalar(15, 118, 110);
  if (points.size() >= 4) {
    for (std::size_t index = 0; index < points.size(); ++index) {
      cv::line(output, points[index], points[(index + 1) % points.size()], accent, 3);
    }
  }

  const auto label = decoded.empty() ? std::string{"QR not found"} : "QR: " + decoded.substr(0, 96);
  if (output.cols > 48 && output.rows > 56) {
    cv::rectangle(output, cv::Rect(12, 12, std::min(output.cols - 24, 640), 42), cv::Scalar(248, 250, 252), cv::FILLED);
    cv::putText(output, label, cv::Point(24, 40), cv::FONT_HERSHEY_SIMPLEX, 0.7, accent, 2, cv::LINE_AA);
  }
  return output;
}

cv::Mat apply_calibration_board_preview(const cv::Mat& source, const nlohmann::json& params) {
  const int board_width = std::clamp(json_int(params, "boardWidth", 9), 2, 32);
  const int board_height = std::clamp(json_int(params, "boardHeight", 6), 2, 32);
  const cv::Size board_size(board_width, board_height);
  std::vector<cv::Point2f> corners;
  const bool found = cv::findChessboardCorners(to_gray(source), board_size, corners);

  cv::Mat output = to_bgr(source);
  cv::drawChessboardCorners(output, board_size, corners, found);
  const auto label = found ? "Calibration board found" : "Calibration board not found";
  const auto color = found ? cv::Scalar(15, 118, 110) : cv::Scalar(194, 65, 12);
  if (output.cols > 48 && output.rows > 56) {
    cv::rectangle(output, cv::Rect(12, 12, std::min(output.cols - 24, 520), 42), cv::Scalar(248, 250, 252), cv::FILLED);
    cv::putText(output, label, cv::Point(24, 40), cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2, cv::LINE_AA);
  }
  return output;
}

}  // namespace

cv::Mat apply_image_operation(
    const cv::Mat& original, const cv::Mat& source, const std::string& operation, const nlohmann::json& params) {
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
    const cv::Mat kernel =
        (cv::Mat_<double>(3, 3) << 0, -strength, 0, -strength, 1 + 4 * strength, -strength, 0, -strength, 0);
    cv::filter2D(source, output, source.depth(), kernel);
    return output;
  }

  if (operation == "threshold") {
    cv::Mat output;
    cv::threshold(
        to_gray(source),
        output,
        std::clamp(json_double(params, "threshold", 128.0), 0.0, 255.0),
        255,
        cv::THRESH_BINARY);
    return output;
  }

  if (operation == "edgeDetect") {
    cv::Mat output;
    cv::Canny(
        to_gray(source),
        output,
        std::clamp(json_double(params, "low", 80.0), 0.0, 255.0),
        std::clamp(json_double(params, "high", 160.0), 0.0, 255.0));
    return output;
  }

  if (operation == "contourDetect") {
    cv::Mat edges;
    cv::Canny(
        to_gray(source),
        edges,
        std::clamp(json_double(params, "low", 80.0), 0.0, 255.0),
        std::clamp(json_double(params, "high", 160.0), 0.0, 255.0));
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

  if (operation == "featureAlign") {
    return apply_feature_alignment(original, source, params);
  }

  if (operation == "eccAlign") {
    return apply_ecc_alignment(original, source, params);
  }

  if (operation == "qrScan") {
    return apply_qr_overlay(source);
  }

  if (operation == "calibrationBoard") {
    return apply_calibration_board_preview(source, params);
  }

  throw std::runtime_error("Unsupported image operation: " + operation);
}

}  // namespace app
