#include "CalibrationService.h"

#include "../common/OpenCvUtils.h"

#include <algorithm>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <vector>

namespace app {

namespace {

nlohmann::json mat_to_json(const cv::Mat& matrix) {
  auto rows = nlohmann::json::array();
  for (int row = 0; row < matrix.rows; ++row) {
    auto values = nlohmann::json::array();
    for (int col = 0; col < matrix.cols; ++col) {
      values.push_back(matrix.at<double>(row, col));
    }
    rows.push_back(values);
  }
  return rows;
}

nlohmann::json vector_to_json(const cv::Mat& vector) {
  auto values = nlohmann::json::array();
  const cv::Mat flattened = vector.reshape(1, 1);
  for (int index = 0; index < flattened.cols; ++index) {
    values.push_back(flattened.at<double>(0, index));
  }
  return values;
}

}  // namespace

CalibrationService::CalibrationService(ImageResultStore& image_store, CalibrationStore& calibration_store)
    : image_store_(image_store), calibration_store_(calibration_store) {}

nlohmann::json CalibrationService::calibrate_from_image(
    const std::string& result_id, int board_width, int board_height, double square_size) {
  board_width = std::clamp(board_width, 2, 32);
  board_height = std::clamp(board_height, 2, 32);
  square_size = std::clamp(square_size, 0.001, 1000.0);

  const auto metadata = image_store_.get(result_id);
  if (!metadata) {
    throw std::runtime_error("Image result was not found.");
  }

  const auto image = image_store_.preview(result_id, "result");
  if (!image || image->empty()) {
    throw std::runtime_error("Image result preview was not available.");
  }

  const cv::Size board_size(board_width, board_height);
  std::vector<cv::Point2f> corners;
  const bool found = cv::findChessboardCorners(to_gray(*image), board_size, corners);
  if (!found) {
    throw std::runtime_error("Calibration chessboard corners were not found.");
  }

  cv::Mat gray = to_gray(*image);
  cv::cornerSubPix(
      gray,
      corners,
      cv::Size(11, 11),
      cv::Size(-1, -1),
      cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.001));

  std::vector<cv::Point3f> object_corners;
  object_corners.reserve(static_cast<std::size_t>(board_width * board_height));
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      object_corners.emplace_back(static_cast<float>(x * square_size), static_cast<float>(y * square_size), 0.0F);
    }
  }

  std::vector<std::vector<cv::Point3f>> object_points = {object_corners};
  std::vector<std::vector<cv::Point2f>> image_points = {corners};
  cv::Mat camera_matrix = cv::Mat::eye(3, 3, CV_64F);
  cv::Mat distortion_coefficients;
  std::vector<cv::Mat> rotation_vectors;
  std::vector<cv::Mat> translation_vectors;
  const double rms = cv::calibrateCamera(
      object_points,
      image_points,
      image->size(),
      camera_matrix,
      distortion_coefficients,
      rotation_vectors,
      translation_vectors);

  const nlohmann::json calibration = {
      {"imageResultId", result_id},
      {"imageName", metadata->value("name", std::string{"image"})},
      {"imageSize", {{"width", image->cols}, {"height", image->rows}}},
      {"board", {{"width", board_width}, {"height", board_height}, {"squareSize", square_size}}},
      {"cornerCount", corners.size()},
      {"rmsReprojectionError", rms},
      {"cameraMatrix", mat_to_json(camera_matrix)},
      {"distortionCoefficients", vector_to_json(distortion_coefficients)},
      {"rotationVector", rotation_vectors.empty() ? nlohmann::json::array() : vector_to_json(rotation_vectors.front())},
      {"translationVector",
       translation_vectors.empty() ? nlohmann::json::array() : vector_to_json(translation_vectors.front())},
  };

  return calibration_store_.record(calibration);
}

}  // namespace app
