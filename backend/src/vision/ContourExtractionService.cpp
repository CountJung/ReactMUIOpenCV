#include "ContourExtractionService.h"

#include "../common/OpenCvUtils.h"
#include "../image/ImageFilters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <vector>

namespace app {

namespace {

template <typename T> nlohmann::json point_to_json(const cv::Point_<T>& point) {
  return {{"x", point.x}, {"y", point.y}};
}

std::array<cv::Point2f, 4> order_quad_points(const std::vector<cv::Point>& points) {
  if (points.size() != 4) {
    throw std::runtime_error("Contour candidate must have exactly four points.");
  }

  std::array<cv::Point2f, 4> ordered{};
  std::vector<cv::Point2f> float_points;
  float_points.reserve(points.size());
  for (const auto& point : points) {
    float_points.emplace_back(static_cast<float>(point.x), static_cast<float>(point.y));
  }

  const auto sum_compare = [](const cv::Point2f& left, const cv::Point2f& right) {
    return left.x + left.y < right.x + right.y;
  };
  const auto diff_compare = [](const cv::Point2f& left, const cv::Point2f& right) {
    return left.x - left.y < right.x - right.y;
  };

  ordered[0] = *std::min_element(float_points.begin(), float_points.end(), sum_compare);
  ordered[2] = *std::max_element(float_points.begin(), float_points.end(), sum_compare);
  ordered[1] = *std::max_element(float_points.begin(), float_points.end(), diff_compare);
  ordered[3] = *std::min_element(float_points.begin(), float_points.end(), diff_compare);
  return ordered;
}

std::array<cv::Point2f, 4> order_quad_points(const std::vector<cv::Point2f>& points) {
  if (points.size() != 4) {
    throw std::runtime_error("Contour candidate must have exactly four points.");
  }

  std::array<cv::Point2f, 4> ordered{};
  const auto sum_compare = [](const cv::Point2f& left, const cv::Point2f& right) {
    return left.x + left.y < right.x + right.y;
  };
  const auto diff_compare = [](const cv::Point2f& left, const cv::Point2f& right) {
    return left.x - left.y < right.x - right.y;
  };

  ordered[0] = *std::min_element(points.begin(), points.end(), sum_compare);
  ordered[2] = *std::max_element(points.begin(), points.end(), sum_compare);
  ordered[1] = *std::max_element(points.begin(), points.end(), diff_compare);
  ordered[3] = *std::min_element(points.begin(), points.end(), diff_compare);
  return ordered;
}

nlohmann::json ordered_points_to_json(const std::array<cv::Point2f, 4>& points) {
  auto result = nlohmann::json::array();
  for (const auto& point : points) {
    result.push_back(point_to_json(point));
  }
  return result;
}

std::vector<cv::Point> approximate_quad(const std::vector<cv::Point>& contour, double epsilon_factor) {
  const double perimeter = cv::arcLength(contour, true);
  const std::array<double, 5> factors = {
      std::clamp(epsilon_factor, 0.005, 0.12),
      0.018,
      0.026,
      0.038,
      0.055,
  };
  for (const double factor : factors) {
    std::vector<cv::Point> approximation;
    cv::approxPolyDP(contour, approximation, perimeter * factor, true);
    if (approximation.size() == 4 && cv::isContourConvex(approximation)) {
      return approximation;
    }
  }
  return {};
}

nlohmann::json contour_candidate_from_points(
    const std::vector<cv::Point>& contour,
    const std::array<cv::Point2f, 4>& ordered_points,
    const std::string& source,
    double image_area,
    std::size_t source_index) {
  std::vector<cv::Point> integer_points;
  integer_points.reserve(ordered_points.size());
  for (const auto& point : ordered_points) {
    integer_points.emplace_back(cvRound(point.x), cvRound(point.y));
  }

  const double area = std::abs(cv::contourArea(integer_points));
  const cv::Rect bounds = cv::boundingRect(integer_points);
  const double bounds_area = static_cast<double>(std::max(1, bounds.width * bounds.height));
  const double rectangularity = std::clamp(area / bounds_area, 0.0, 1.0);
  const double area_ratio = area / image_area;
  const double perimeter = cv::arcLength(contour.empty() ? integer_points : contour, true);
  const double score = area_ratio * 100.0 + rectangularity * 20.0;

  return {
      {"candidateId", ""},
      {"index", 0},
      {"sourceIndex", source_index},
      {"source", source},
      {"points", ordered_points_to_json(ordered_points)},
      {"boundingBox", rect_to_json(bounds)},
      {"area", area},
      {"areaRatio", area_ratio},
      {"perimeter", perimeter},
      {"score", score},
  };
}

std::array<cv::Point2f, 4> rotated_rect_points(const cv::RotatedRect& box) {
  std::array<cv::Point2f, 4> points{};
  box.points(points.data());
  return order_quad_points(std::vector<cv::Point2f>(points.begin(), points.end()));
}

nlohmann::json make_extract_params(const nlohmann::json& candidate) {
  if (!candidate.is_object() || !candidate.contains("points")) {
    throw std::runtime_error("A selected contour candidate with points is required.");
  }
  return {{"candidateId", candidate.value("candidateId", std::string{"manual"})}, {"points", candidate["points"]}};
}

}  // namespace

ContourExtractionService::ContourExtractionService(ImageResultStore& image_store)
    : image_store_(image_store) {}

nlohmann::json
ContourExtractionService::detect_candidates(const std::string& image_result_id, const nlohmann::json& params) const {
  const auto metadata = image_store_.get(image_result_id);
  if (!metadata) {
    throw std::runtime_error("Image result was not found.");
  }

  const auto preview = image_store_.preview(image_result_id, "result");
  if (!preview || preview->empty()) {
    throw std::runtime_error("Image preview was not available.");
  }

  const int max_candidates = std::clamp(params.at("maxCandidates").get<int>(), 1, 80);
  const double low_threshold = std::clamp(params.at("low").get<double>(), 1.0, 255.0);
  const double high_threshold = std::clamp(params.at("high").get<double>(), low_threshold + 1.0, 255.0);
  const double min_area_ratio = std::clamp(params.at("minAreaRatio").get<double>(), 0.0001, 0.5);
  const double epsilon_factor = std::clamp(params.at("epsilon").get<double>(), 0.005, 0.12);
  const auto retrieval = params.at("retrieval").get<std::string>();
  const int retrieval_mode = retrieval == "external" ? cv::RETR_EXTERNAL : cv::RETR_LIST;

  const cv::Mat bgr = to_bgr(*preview);
  cv::Mat blurred;
  cv::GaussianBlur(to_gray(bgr), blurred, cv::Size(5, 5), 0.0);

  cv::Mat edges;
  cv::Canny(blurred, edges, low_threshold, high_threshold);
  cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, cv::Mat::ones(5, 5, CV_8U), cv::Point(-1, -1), 1);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(edges, contours, retrieval_mode, cv::CHAIN_APPROX_SIMPLE);

  const double image_area = static_cast<double>(std::max(1, bgr.cols * bgr.rows));
  const double min_area = image_area * min_area_ratio;
  std::vector<nlohmann::json> ranked_candidates;

  for (std::size_t contour_index = 0; contour_index < contours.size(); ++contour_index) {
    const auto& contour = contours[contour_index];
    if (contour.size() < 4 || std::abs(cv::contourArea(contour)) < min_area) {
      continue;
    }

    const auto quad = approximate_quad(contour, epsilon_factor);
    if (!quad.empty()) {
      const auto ordered_points = order_quad_points(quad);
      if (std::abs(cv::contourArea(quad)) >= min_area) {
        ranked_candidates.push_back(
            contour_candidate_from_points(contour, ordered_points, "approxPolyDP", image_area, contour_index));
        continue;
      }
    }

    const auto rect = cv::minAreaRect(contour);
    if (rect.size.width < 8.0F || rect.size.height < 8.0F) {
      continue;
    }
    const auto ordered_points = rotated_rect_points(rect);
    ranked_candidates.push_back(
        contour_candidate_from_points(contour, ordered_points, "minAreaRect", image_area, contour_index));
  }

  std::sort(ranked_candidates.begin(), ranked_candidates.end(), [](const auto& left, const auto& right) {
    return left.value("score", 0.0) > right.value("score", 0.0);
  });
  auto candidates = nlohmann::json::array();
  for (std::size_t index = 0; index < ranked_candidates.size() && index < static_cast<std::size_t>(max_candidates);
       ++index) {
    ranked_candidates[index]["candidateId"] = "candidate-" + std::to_string(index + 1);
    ranked_candidates[index]["index"] = index + 1;
    candidates.push_back(ranked_candidates[index]);
  }

  return {
      {"source", *metadata},
      {"method",
       {{"edge", "canny"},
        {"retrieval", retrieval},
        {"low", low_threshold},
        {"high", high_threshold},
        {"minAreaRatio", min_area_ratio},
        {"epsilon", epsilon_factor},
        {"contourCount", contours.size()}}},
      {"candidates", candidates},
  };
}

cv::Mat
ContourExtractionService::preview_candidate(const std::string& image_result_id, const nlohmann::json& candidate) const {
  const auto preview = image_store_.preview(image_result_id, "result");
  if (!preview || preview->empty()) {
    throw std::runtime_error("Image preview was not available.");
  }

  return apply_image_operation(*preview, *preview, "perspectiveExtract", make_extract_params(candidate)).image;
}

nlohmann::json
ContourExtractionService::extract_candidate(const std::string& image_result_id, const nlohmann::json& candidate) const {
  return image_store_.process(image_result_id, "perspectiveExtract", make_extract_params(candidate));
}

}  // namespace app
