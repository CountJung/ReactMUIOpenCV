#include "ImageFilters.h"

#include "../common/OpenCvUtils.h"
#include "../common/StringUtils.h"
#include "ImageDnnFilters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/video/tracking.hpp>
#include <stdexcept>
#include <utility>
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

ImageOperationResult image_only(cv::Mat image) {
  return {std::move(image), nlohmann::json::object()};
}

template <typename T> nlohmann::json point_to_json(const cv::Point_<T>& point) {
  return {{"x", point.x}, {"y", point.y}};
}

std::array<cv::Point2f, 4> order_quad_points(std::array<cv::Point2f, 4> points) {
  const auto sum_compare = [](const cv::Point2f& left, const cv::Point2f& right) {
    return left.x + left.y < right.x + right.y;
  };
  const auto diff_compare = [](const cv::Point2f& left, const cv::Point2f& right) {
    return left.x - left.y < right.x - right.y;
  };

  std::array<cv::Point2f, 4> ordered{};
  ordered[0] = *std::min_element(points.begin(), points.end(), sum_compare);
  ordered[2] = *std::max_element(points.begin(), points.end(), sum_compare);
  ordered[1] = *std::max_element(points.begin(), points.end(), diff_compare);
  ordered[3] = *std::min_element(points.begin(), points.end(), diff_compare);
  return ordered;
}

std::array<cv::Point2f, 4> quad_points_from_json(const nlohmann::json& params, const cv::Size& size) {
  const auto points = params.value("points", nlohmann::json::array());
  if (!points.is_array() || points.size() != 4) {
    throw std::runtime_error("Perspective extraction requires exactly four selected contour points.");
  }

  std::array<cv::Point2f, 4> parsed{};
  for (std::size_t index = 0; index < parsed.size(); ++index) {
    const auto& point = points[index];
    if (!point.is_object() || !point.contains("x") || !point.contains("y")) {
      throw std::runtime_error("Perspective extraction points must contain x and y values.");
    }
    const auto x = std::clamp(point.value("x", 0.0), 0.0, static_cast<double>(std::max(0, size.width - 1)));
    const auto y = std::clamp(point.value("y", 0.0), 0.0, static_cast<double>(std::max(0, size.height - 1)));
    parsed[index] = cv::Point2f(static_cast<float>(x), static_cast<float>(y));
  }
  return order_quad_points(parsed);
}

double point_distance(const cv::Point2f& left, const cv::Point2f& right) {
  return cv::norm(left - right);
}

cv::Rect bounding_rect_from_quad(const std::array<cv::Point2f, 4>& points) {
  std::vector<cv::Point> integer_points;
  integer_points.reserve(points.size());
  for (const auto& point : points) {
    integer_points.emplace_back(cvRound(point.x), cvRound(point.y));
  }
  return cv::boundingRect(integer_points);
}

cv::Rect centered_rect(const cv::Mat& source, const nlohmann::json& params, double scale = 0.35) {
  if (source.cols < 4 || source.rows < 4) {
    throw std::runtime_error("Image is too small for an ROI-based operation.");
  }

  const int fallback_width = std::max(8, static_cast<int>(source.cols * scale));
  const int fallback_height = std::max(8, static_cast<int>(source.rows * scale));
  const int width = std::clamp(json_int(params, "width", fallback_width), 4, std::max(4, source.cols));
  const int height = std::clamp(json_int(params, "height", fallback_height), 4, std::max(4, source.rows));
  const int fallback_x = std::max(0, (source.cols - width) / 2);
  const int fallback_y = std::max(0, (source.rows - height) / 2);
  const int x = std::clamp(json_int(params, "x", fallback_x), 0, source.cols - width);
  const int y = std::clamp(json_int(params, "y", fallback_y), 0, source.rows - height);
  return {x, y, width, height};
}

cv::Mat normalize_preview_float(const cv::Mat& source) {
  cv::Mat normalized;
  cv::normalize(source, normalized, 0.0, 1.0, cv::NORM_MINMAX);
  cv::Mat output;
  normalized.convertTo(output, CV_8UC3, 255.0);
  return output;
}

cv::Mat apply_gamma(const cv::Mat& source, double gamma) {
  cv::Mat normalized;
  to_bgr(source).convertTo(normalized, CV_32F, 1.0 / 255.0);
  cv::pow(normalized, std::max(0.05, gamma), normalized);
  cv::Mat output;
  normalized.convertTo(output, CV_8UC3, 255.0);
  return output;
}

ImageOperationResult apply_inpaint_filter(const cv::Mat& source, const nlohmann::json& params) {
  const auto mask_mode = params.value("maskMode", std::string{"edges"});
  const double threshold = std::clamp(json_double(params, "threshold", 220.0), 0.0, 255.0);
  const double radius = std::clamp(json_double(params, "radius", 3.0), 1.0, 24.0);

  cv::Mat mask;
  if (mask_mode == "center") {
    mask = cv::Mat::zeros(source.size(), CV_8UC1);
    cv::rectangle(mask, centered_rect(source, params, 0.24), cv::Scalar(255), cv::FILLED);
  } else if (mask_mode == "dark") {
    cv::threshold(to_gray(source), mask, threshold, 255, cv::THRESH_BINARY_INV);
  } else if (mask_mode == "bright") {
    cv::threshold(to_gray(source), mask, threshold, 255, cv::THRESH_BINARY);
  } else {
    cv::Canny(to_gray(source), mask, 80, 180);
    cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 1);
  }

  cv::Mat output;
  cv::inpaint(to_bgr(source), mask, output, radius, cv::INPAINT_TELEA);
  return {output, {{"composition", {{"operation", "inpaint"}, {"maskMode", mask_mode}, {"radius", radius}}}}};
}

ImageOperationResult apply_seamless_clone_filter(const cv::Mat& source, const nlohmann::json& params) {
  const cv::Mat bgr = to_bgr(source);
  const cv::Rect roi = centered_rect(bgr, params, 0.32);
  const int half_width = std::max(1, roi.width / 2);
  const int half_height = std::max(1, roi.height / 2);
  const int target_x = std::clamp(json_int(params, "targetX", bgr.cols / 2), half_width, bgr.cols - half_width);
  const int target_y = std::clamp(json_int(params, "targetY", bgr.rows / 2), half_height, bgr.rows - half_height);
  const auto mode = params.value("mode", std::string{"mixed"});
  const int flags = mode == "normal" ? cv::NORMAL_CLONE : cv::MIXED_CLONE;

  const cv::Mat patch = bgr(roi).clone();
  const cv::Mat mask(patch.size(), CV_8UC1, cv::Scalar(255));
  cv::Mat output;
  cv::seamlessClone(patch, bgr, mask, cv::Point(target_x, target_y), output, flags);
  return {
      output,
      {{"composition",
        {{"operation", "seamlessClone"},
         {"sourceRoi", rect_to_json(roi)},
         {"target", point_to_json(cv::Point(target_x, target_y))},
         {"mode", mode}}}}};
}

ImageOperationResult
apply_alpha_blend_filter(const cv::Mat& original, const cv::Mat& source, const nlohmann::json& params) {
  const double alpha = std::clamp(json_double(params, "alpha", 0.5), 0.0, 1.0);
  cv::Mat baseline = to_bgr(original.empty() ? source : original);
  cv::Mat candidate = to_bgr(source);
  if (baseline.size() != candidate.size()) {
    cv::resize(baseline, baseline, candidate.size(), 0, 0, cv::INTER_AREA);
  }

  cv::Mat output;
  cv::addWeighted(candidate, alpha, baseline, 1.0 - alpha, 0.0, output);
  return {output, {{"composition", {{"operation", "alphaBlend"}, {"alpha", alpha}}}}};
}

ImageOperationResult apply_exposure_fusion_filter(const cv::Mat& source, const nlohmann::json& params) {
  std::vector<cv::Mat> exposures = {
      apply_gamma(source, std::clamp(json_double(params, "darkGamma", 1.8), 0.2, 4.0)),
      to_bgr(source),
      apply_gamma(source, std::clamp(json_double(params, "brightGamma", 0.55), 0.2, 4.0)),
  };

  cv::Mat fusion;
  cv::createMergeMertens(
      static_cast<float>(std::clamp(json_double(params, "contrast", 1.0), 0.0, 3.0)),
      static_cast<float>(std::clamp(json_double(params, "saturation", 1.0), 0.0, 3.0)),
      static_cast<float>(std::clamp(json_double(params, "exposure", 0.0), 0.0, 3.0)))
      ->process(exposures, fusion);
  cv::Mat output;
  fusion.convertTo(output, CV_8UC3, 255.0);
  return {output, {{"composition", {{"operation", "exposureFusion"}, {"exposureCount", exposures.size()}}}}};
}

ImageOperationResult apply_hdr_tonemap_filter(const cv::Mat& source, const nlohmann::json& params) {
  cv::Mat hdr;
  to_bgr(source).convertTo(hdr, CV_32F, std::clamp(json_double(params, "exposureScale", 2.2), 0.5, 8.0) / 255.0);
  cv::Mat tonemapped;
  cv::createTonemapReinhard(
      static_cast<float>(std::clamp(json_double(params, "gamma", 1.0), 0.2, 3.0)),
      static_cast<float>(std::clamp(json_double(params, "intensity", 0.0), -8.0, 8.0)),
      static_cast<float>(std::clamp(json_double(params, "lightAdapt", 0.8), 0.0, 1.0)),
      static_cast<float>(std::clamp(json_double(params, "colorAdapt", 0.0), 0.0, 1.0)))
      ->process(hdr, tonemapped);
  return {normalize_preview_float(tonemapped), {{"composition", {{"operation", "hdrTonemap"}}}}};
}

ImageOperationResult apply_stylization_filter(const cv::Mat& source, const nlohmann::json& params) {
  cv::Mat output;
  cv::stylization(
      to_bgr(source),
      output,
      static_cast<float>(std::clamp(json_double(params, "sigmaS", 60.0), 1.0, 200.0)),
      static_cast<float>(std::clamp(json_double(params, "sigmaR", 0.45), 0.01, 1.0)));
  return {output, {{"composition", {{"operation", "stylization"}}}}};
}

ImageOperationResult apply_pencil_sketch_filter(const cv::Mat& source, const nlohmann::json& params) {
  cv::Mat grayscale;
  cv::Mat color;
  cv::pencilSketch(
      to_bgr(source),
      grayscale,
      color,
      static_cast<float>(std::clamp(json_double(params, "sigmaS", 60.0), 1.0, 200.0)),
      static_cast<float>(std::clamp(json_double(params, "sigmaR", 0.07), 0.01, 1.0)),
      static_cast<float>(std::clamp(json_double(params, "shade", 0.02), 0.0, 0.2)));
  const auto mode = params.value("mode", std::string{"color"});
  return {mode == "gray" ? grayscale : color, {{"composition", {{"operation", "pencilSketch"}, {"mode", mode}}}}};
}

ImageOperationResult apply_perspective_extract_filter(const cv::Mat& source, const nlohmann::json& params) {
  const auto source_points = quad_points_from_json(params, source.size());
  const double top_width = point_distance(source_points[0], source_points[1]);
  const double bottom_width = point_distance(source_points[3], source_points[2]);
  const double right_height = point_distance(source_points[1], source_points[2]);
  const double left_height = point_distance(source_points[0], source_points[3]);

  const int default_width = std::clamp(cvRound(std::max(top_width, bottom_width)), 16, 8192);
  const int default_height = std::clamp(cvRound(std::max(left_height, right_height)), 16, 8192);
  const int output_width = std::clamp(json_int(params, "outputWidth", default_width), 16, 8192);
  const int output_height = std::clamp(json_int(params, "outputHeight", default_height), 16, 8192);

  const std::array<cv::Point2f, 4> target_points = {
      cv::Point2f(0.0F, 0.0F),
      cv::Point2f(static_cast<float>(output_width - 1), 0.0F),
      cv::Point2f(static_cast<float>(output_width - 1), static_cast<float>(output_height - 1)),
      cv::Point2f(0.0F, static_cast<float>(output_height - 1)),
  };

  const std::vector<cv::Point2f> source_vector(source_points.begin(), source_points.end());
  const std::vector<cv::Point2f> target_vector(target_points.begin(), target_points.end());
  const auto transform = cv::getPerspectiveTransform(source_vector, target_vector);
  cv::Mat output;
  cv::warpPerspective(to_bgr(source), output, transform, cv::Size(output_width, output_height), cv::INTER_LINEAR);

  const double area = std::abs(cv::contourArea(source_vector));
  return {
      output,
      {{"contourExtraction",
        {{"operation", "perspectiveExtract"},
         {"candidateId", params.value("candidateId", std::string{"manual"})},
         {"sourcePoints",
          {point_to_json(source_points[0]),
           point_to_json(source_points[1]),
           point_to_json(source_points[2]),
           point_to_json(source_points[3])}},
         {"outputSize", {{"width", output_width}, {"height", output_height}}},
         {"area", area},
         {"boundingBox", rect_to_json(bounding_rect_from_quad(source_points))}}}}};
}

void draw_sample_text(
    cv::Mat& target,
    const std::string& text,
    const cv::Point& origin,
    double scale,
    const cv::Scalar& color,
    int thickness = 1) {
  cv::putText(target, text, origin, cv::FONT_HERSHEY_SIMPLEX, scale, color, thickness, cv::LINE_AA);
}

cv::Size default_sample_tile_size(const cv::Mat& source) {
  if (source.empty()) {
    return {350, 460};
  }

  const double aspect = static_cast<double>(source.cols) / static_cast<double>(std::max(1, source.rows));
  if (aspect >= 1.0) {
    const int width = std::clamp(520, 220, 640);
    const int height = std::clamp(static_cast<int>(std::round(width / aspect)), 260, 760);
    return {width, height};
  }

  const int height = 460;
  const int width = std::clamp(static_cast<int>(std::round(height * aspect)), 220, 640);
  return {width, height};
}

ImageOperationResult apply_vision_sample_board(const cv::Mat& source, const nlohmann::json& params) {
  const cv::Size default_tile = default_sample_tile_size(source);
  const int tile_width = std::clamp(json_int(params, "tileWidth", default_tile.width), 220, 640);
  const int tile_height = std::clamp(json_int(params, "tileHeight", default_tile.height), 260, 760);
  constexpr int label_height = 44;
  constexpr int pad = 18;
  constexpr int header_height = 74;
  constexpr int footer_height = 46;
  constexpr int cols = 3;
  constexpr int rows = 2;
  const int board_width = cols * tile_width + (cols + 1) * pad;
  const int board_height = header_height + rows * (tile_height + label_height) + (rows + 1) * pad + footer_height;

  cv::Mat board(board_height, board_width, CV_8UC3, cv::Scalar(26, 32, 44));
  const cv::Scalar text_color(244, 247, 251);
  const cv::Scalar muted_color(175, 185, 198);
  const cv::Scalar card_color(38, 47, 62);

  draw_sample_text(board, "OpenCV Vision Samples", cv::Point(pad, 42), 1.08, text_color, 2);
  draw_sample_text(
      board,
      "Image Lab and Pipeline Flow can turn one image into inspectable processing results.",
      cv::Point(pad, 66),
      0.48,
      muted_color);

  const cv::Mat work = fit_cover(source, tile_width, tile_height);
  const cv::Mat gray = to_gray(work);
  cv::Mat blur;
  cv::GaussianBlur(gray, blur, cv::Size(5, 5), 0);

  cv::Mat clahe;
  cv::createCLAHE(2.2, cv::Size(8, 8))->apply(gray, clahe);
  cv::Mat clahe_bgr;
  cv::cvtColor(clahe, clahe_bgr, cv::COLOR_GRAY2BGR);

  cv::Mat edges;
  cv::Canny(blur, edges, 45, 130);
  cv::Mat edges_dilated;
  cv::dilate(edges, edges_dilated, cv::Mat::ones(2, 2, CV_8U), cv::Point(-1, -1), 1);
  cv::Mat edge_overlay;
  cv::addWeighted(work, 0.34, cv::Mat::zeros(work.size(), work.type()), 0.66, 0.0, edge_overlay);
  edge_overlay.setTo(cv::Scalar(36, 215, 255), edges_dilated);

  cv::Mat adaptive;
  cv::adaptiveThreshold(blur, adaptive, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 31, 4);
  cv::Mat adaptive_bgr;
  cv::cvtColor(adaptive, adaptive_bgr, cv::COLOR_GRAY2BGR);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  std::vector<std::vector<cv::Point>> filtered;
  std::copy_if(contours.begin(), contours.end(), std::back_inserter(filtered), [](const auto& contour) {
    const auto area = cv::contourArea(contour);
    return area > 55.0 && area < 9000.0;
  });

  cv::Mat contour_overlay;
  cv::addWeighted(work, 0.62, cv::Mat::zeros(work.size(), work.type()), 0.38, 0.0, contour_overlay);
  cv::drawContours(contour_overlay, filtered, -1, cv::Scalar(88, 235, 160), 1, cv::LINE_AA);
  std::sort(filtered.begin(), filtered.end(), [](const auto& left, const auto& right) {
    return cv::contourArea(left) > cv::contourArea(right);
  });
  for (std::size_t index = 0; index < filtered.size() && index < 8; ++index) {
    const auto bounds = cv::boundingRect(filtered[index]);
    if (bounds.width > 10 && bounds.height > 10) {
      cv::rectangle(contour_overlay, bounds, cv::Scalar(255, 205, 72), 1, cv::LINE_AA);
    }
  }

  auto orb = cv::ORB::create(180, 1.2F, 8, 12);
  std::vector<cv::KeyPoint> keypoints;
  orb->detect(gray, keypoints);
  std::sort(keypoints.begin(), keypoints.end(), [](const auto& left, const auto& right) {
    return left.response > right.response;
  });
  if (keypoints.size() > 90) {
    keypoints.resize(90);
  }
  cv::Mat feature_keypoints;
  cv::drawKeypoints(
      work, keypoints, feature_keypoints, cv::Scalar(255, 105, 210), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  cv::Mat feature_overlay;
  cv::addWeighted(work, 0.72, feature_keypoints, 0.58, 0.0, feature_overlay);

  const std::array<std::pair<std::string, cv::Mat>, 6> samples = {
      std::pair<std::string, cv::Mat>{"Original input", work},
      {"Grayscale + CLAHE", clahe_bgr},
      {"Canny edge map", edge_overlay},
      {"Adaptive threshold", adaptive_bgr},
      {"Contour overlay", contour_overlay},
      {"ORB feature points", feature_overlay},
  };

  for (std::size_t index = 0; index < samples.size(); ++index) {
    const int row = static_cast<int>(index) / cols;
    const int col = static_cast<int>(index) % cols;
    const int x = pad + col * (tile_width + pad);
    const int y = header_height + pad + row * (tile_height + label_height + pad);
    cv::rectangle(
        board,
        cv::Rect(x - 1, y - 1, tile_width + 2, label_height + tile_height + 2),
        cv::Scalar(58, 70, 89),
        1,
        cv::LINE_AA);
    board(cv::Rect(x, y, tile_width, label_height)).setTo(card_color);
    draw_sample_text(board, samples[index].first, cv::Point(x + 14, y + 29), 0.55, text_color);
    samples[index].second.copyTo(board(cv::Rect(x, y + label_height, tile_width, tile_height)));
  }

  draw_sample_text(
      board,
      "Generated inside ReactMUIOpenCV with OpenCV: CLAHE, Canny, adaptive threshold, contours, and ORB.",
      cv::Point(pad, board_height - 22),
      0.45,
      muted_color);

  return {
      board,
      {{"sampleBoard",
        {{"operation", "visionSampleBoard"},
         {"tileWidth", tile_width},
         {"tileHeight", tile_height},
         {"contourCount", filtered.size()},
         {"orbKeypoints", keypoints.size()},
         {"stages", {"original", "clahe", "canny", "adaptiveThreshold", "contours", "orb"}}}}}};
}

std::vector<std::vector<cv::Point>> find_shape_contours(const cv::Mat& source, const nlohmann::json& params) {
  const double threshold = std::clamp(json_double(params, "threshold", 128.0), 0.0, 255.0);
  const int min_area = std::clamp(json_int(params, "minArea", 80), 1, 1000000);
  const auto polarity = params.value("polarity", std::string{"dark"});

  cv::Mat binary;
  cv::threshold(
      to_gray(source), binary, threshold, 255, polarity == "light" ? cv::THRESH_BINARY : cv::THRESH_BINARY_INV);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  std::erase_if(contours, [&](const auto& contour) {
    return std::abs(cv::contourArea(contour)) < static_cast<double>(min_area);
  });
  std::sort(contours.begin(), contours.end(), [](const auto& left, const auto& right) {
    return std::abs(cv::contourArea(left)) > std::abs(cv::contourArea(right));
  });
  return contours;
}

ImageOperationResult apply_blob_centroid(const cv::Mat& source, const nlohmann::json& params) {
  const auto contours = find_shape_contours(source, params);
  cv::Mat output = to_bgr(source);
  auto blobs = nlohmann::json::array();

  const int max_shapes = std::clamp(json_int(params, "maxShapes", 24), 1, 128);
  for (std::size_t index = 0; index < contours.size() && index < static_cast<std::size_t>(max_shapes); ++index) {
    const auto& contour = contours[index];
    const auto moments = cv::moments(contour);
    if (std::abs(moments.m00) < 1e-6) {
      continue;
    }

    const cv::Point2d centroid(moments.m10 / moments.m00, moments.m01 / moments.m00);
    const cv::Rect bounds = cv::boundingRect(contour);
    const double area = std::abs(cv::contourArea(contour));
    const double perimeter = cv::arcLength(contour, true);
    cv::drawContours(output, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(15, 118, 110), 2);
    cv::circle(output, centroid, 5, cv::Scalar(194, 65, 12), cv::FILLED, cv::LINE_AA);
    cv::putText(
        output,
        std::to_string(index + 1),
        cv::Point(bounds.x, std::max(16, bounds.y - 6)),
        cv::FONT_HERSHEY_SIMPLEX,
        0.55,
        cv::Scalar(37, 99, 235),
        2,
        cv::LINE_AA);

    blobs.push_back(
        {{"index", index + 1},
         {"area", area},
         {"perimeter", perimeter},
         {"centroid", point_to_json(centroid)},
         {"boundingBox", rect_to_json(bounds)}});
  }

  return {
      output,
      {{"shape",
        {{"operation", "blobCentroid"},
         {"contourCount", contours.size()},
         {"shapeCount", blobs.size()},
         {"largestArea", blobs.empty() ? 0.0 : blobs.front().value("area", 0.0)},
         {"blobs", blobs}}}}};
}

ImageOperationResult apply_convex_hull(const cv::Mat& source, const nlohmann::json& params) {
  const auto contours = find_shape_contours(source, params);
  cv::Mat output = to_bgr(source);
  auto hulls = nlohmann::json::array();

  const int max_shapes = std::clamp(json_int(params, "maxShapes", 16), 1, 128);
  for (std::size_t index = 0; index < contours.size() && index < static_cast<std::size_t>(max_shapes); ++index) {
    std::vector<cv::Point> hull;
    cv::convexHull(contours[index], hull);
    const double area = std::abs(cv::contourArea(contours[index]));
    const double hull_area = std::abs(cv::contourArea(hull));
    const double solidity = hull_area > 1e-6 ? area / hull_area : 0.0;
    cv::drawContours(output, std::vector<std::vector<cv::Point>>{contours[index]}, -1, cv::Scalar(37, 99, 235), 2);
    cv::polylines(output, hull, true, cv::Scalar(194, 65, 12), 2, cv::LINE_AA);
    hulls.push_back(
        {{"index", index + 1},
         {"area", area},
         {"hullArea", hull_area},
         {"solidity", solidity},
         {"hullPointCount", hull.size()},
         {"boundingBox", rect_to_json(cv::boundingRect(contours[index]))}});
  }

  return {
      output,
      {{"shape",
        {{"operation", "convexHull"},
         {"contourCount", contours.size()},
         {"shapeCount", hulls.size()},
         {"largestArea", hulls.empty() ? 0.0 : hulls.front().value("area", 0.0)},
         {"hulls", hulls}}}}};
}

ImageOperationResult apply_hu_moments(const cv::Mat& source, const nlohmann::json& params) {
  const auto contours = find_shape_contours(source, params);
  cv::Mat output = to_bgr(source);
  auto shapes = nlohmann::json::array();

  const int max_shapes = std::clamp(json_int(params, "maxShapes", 8), 1, 64);
  for (std::size_t index = 0; index < contours.size() && index < static_cast<std::size_t>(max_shapes); ++index) {
    double raw_hu[7] = {};
    cv::HuMoments(cv::moments(contours[index]), raw_hu);
    auto hu = nlohmann::json::array();
    auto log_hu = nlohmann::json::array();
    for (double value : raw_hu) {
      hu.push_back(value);
      const double magnitude = std::abs(value);
      log_hu.push_back(magnitude > 1e-30 ? -std::copysign(1.0, value) * std::log10(magnitude) : 0.0);
    }

    const double area = std::abs(cv::contourArea(contours[index]));
    const auto bounds = cv::boundingRect(contours[index]);
    cv::drawContours(output, std::vector<std::vector<cv::Point>>{contours[index]}, -1, cv::Scalar(15, 118, 110), 2);
    cv::putText(
        output,
        "Hu " + std::to_string(index + 1),
        cv::Point(bounds.x, std::max(16, bounds.y - 6)),
        cv::FONT_HERSHEY_SIMPLEX,
        0.55,
        cv::Scalar(194, 65, 12),
        2,
        cv::LINE_AA);

    shapes.push_back(
        {{"index", index + 1},
         {"area", area},
         {"boundingBox", rect_to_json(bounds)},
         {"huMoments", hu},
         {"logHuMoments", log_hu}});
  }

  return {
      output,
      {{"shape",
        {{"operation", "huMoments"},
         {"contourCount", contours.size()},
         {"shapeCount", shapes.size()},
         {"largestArea", shapes.empty() ? 0.0 : shapes.front().value("area", 0.0)},
         {"shapes", shapes}}}}};
}

ImageOperationResult apply_hough_transform(const cv::Mat& source, const nlohmann::json& params) {
  const auto mode = params.value("mode", std::string{"lines"});
  cv::Mat gray = to_gray(source);
  cv::Mat output = to_bgr(source);
  cv::Mat edges;
  cv::Canny(
      gray,
      edges,
      std::clamp(json_double(params, "low", 80.0), 0.0, 255.0),
      std::clamp(json_double(params, "high", 160.0), 0.0, 255.0));

  if (mode == "circles") {
    cv::GaussianBlur(gray, gray, cv::Size(9, 9), 2);
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(
        gray,
        circles,
        cv::HOUGH_GRADIENT,
        1.0,
        std::clamp(json_double(params, "minDist", 40.0), 1.0, 1000.0),
        std::clamp(json_double(params, "high", 160.0), 1.0, 255.0),
        std::clamp(json_double(params, "accumulator", 32.0), 1.0, 255.0),
        std::clamp(json_int(params, "minRadius", 0), 0, 4096),
        std::clamp(json_int(params, "maxRadius", 0), 0, 4096));
    auto items = nlohmann::json::array();
    const int max_shapes = std::clamp(json_int(params, "maxShapes", 32), 1, 256);
    for (std::size_t index = 0; index < circles.size() && index < static_cast<std::size_t>(max_shapes); ++index) {
      const auto circle = circles[index];
      const cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
      const int radius = cvRound(circle[2]);
      cv::circle(output, center, radius, cv::Scalar(15, 118, 110), 2, cv::LINE_AA);
      cv::circle(output, center, 3, cv::Scalar(194, 65, 12), cv::FILLED, cv::LINE_AA);
      items.push_back({{"index", index + 1}, {"center", point_to_json(center)}, {"radius", radius}});
    }

    return {
        output,
        {{"shape",
          {{"operation", "houghTransform"},
           {"mode", "circles"},
           {"circleCount", circles.size()},
           {"shapeCount", items.size()},
           {"circles", items}}}}};
  }

  std::vector<cv::Vec4i> lines;
  cv::HoughLinesP(
      edges,
      lines,
      1,
      CV_PI / 180,
      std::clamp(json_int(params, "threshold", 80), 1, 512),
      std::clamp(json_double(params, "minLineLength", 40.0), 1.0, 4096.0),
      std::clamp(json_double(params, "maxLineGap", 12.0), 0.0, 1024.0));
  auto items = nlohmann::json::array();
  const int max_shapes = std::clamp(json_int(params, "maxShapes", 32), 1, 256);
  for (std::size_t index = 0; index < lines.size() && index < static_cast<std::size_t>(max_shapes); ++index) {
    const auto line = lines[index];
    const cv::Point start(line[0], line[1]);
    const cv::Point end(line[2], line[3]);
    const double length = cv::norm(start - end);
    cv::line(output, start, end, cv::Scalar(194, 65, 12), 2, cv::LINE_AA);
    items.push_back(
        {{"index", index + 1}, {"start", point_to_json(start)}, {"end", point_to_json(end)}, {"length", length}});
  }

  return {
      output,
      {{"shape",
        {{"operation", "houghTransform"},
         {"mode", "lines"},
         {"lineCount", lines.size()},
         {"shapeCount", items.size()},
         {"lines", items}}}}};
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

ImageOperationResult apply_image_operation(
    const cv::Mat& original, const cv::Mat& source, const std::string& operation, const nlohmann::json& params) {
  if (source.empty()) {
    throw std::runtime_error("Source image is empty.");
  }

  if (operation == "open") {
    return image_only(source.clone());
  }

  if (operation == "resize") {
    const int width = std::clamp(json_int(params, "width", source.cols), 1, 8192);
    const int height = std::clamp(json_int(params, "height", source.rows), 1, 8192);
    cv::Mat output;
    cv::resize(source, output, cv::Size(width, height), 0, 0, cv::INTER_AREA);
    return image_only(output);
  }

  if (operation == "crop") {
    const int x = std::clamp(json_int(params, "x", 0), 0, std::max(0, source.cols - 1));
    const int y = std::clamp(json_int(params, "y", 0), 0, std::max(0, source.rows - 1));
    const int width = std::clamp(json_int(params, "width", source.cols - x), 1, source.cols - x);
    const int height = std::clamp(json_int(params, "height", source.rows - y), 1, source.rows - y);
    return image_only(source(cv::Rect(x, y, width, height)).clone());
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
    return image_only(output);
  }

  if (operation == "flip") {
    const auto direction = params.value("direction", std::string{"horizontal"});
    const int flip_code = direction == "vertical" ? 0 : direction == "both" ? -1 : 1;
    cv::Mat output;
    cv::flip(source, output, flip_code);
    return image_only(output);
  }

  if (operation == "grayscale") {
    return image_only(to_gray(source));
  }

  if (operation == "blur") {
    const int kernel = odd_kernel(json_int(params, "kernel", 5));
    cv::Mat output;
    cv::blur(source, output, cv::Size(kernel, kernel));
    return image_only(output);
  }

  if (operation == "gaussianBlur") {
    const int kernel = odd_kernel(json_int(params, "kernel", 7));
    cv::Mat output;
    cv::GaussianBlur(source, output, cv::Size(kernel, kernel), 0);
    return image_only(output);
  }

  if (operation == "sharpen") {
    cv::Mat output;
    const double strength = std::clamp(json_double(params, "strength", 1.0), 0.2, 4.0);
    const cv::Mat kernel =
        (cv::Mat_<double>(3, 3) << 0, -strength, 0, -strength, 1 + 4 * strength, -strength, 0, -strength, 0);
    cv::filter2D(source, output, source.depth(), kernel);
    return image_only(output);
  }

  if (operation == "threshold") {
    cv::Mat output;
    cv::threshold(
        to_gray(source),
        output,
        std::clamp(json_double(params, "threshold", 128.0), 0.0, 255.0),
        255,
        cv::THRESH_BINARY);
    return image_only(output);
  }

  if (operation == "edgeDetect") {
    cv::Mat output;
    cv::Canny(
        to_gray(source),
        output,
        std::clamp(json_double(params, "low", 80.0), 0.0, 255.0),
        std::clamp(json_double(params, "high", 160.0), 0.0, 255.0));
    return image_only(output);
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
    return image_only(output);
  }

  if (operation == "histogram") {
    return image_only(apply_histogram_preview(source));
  }

  if (operation == "colorConvert") {
    const auto target = params.value("target", std::string{"hsv"});
    cv::Mat converted;
    if (target == "gray") {
      return image_only(to_gray(source));
    }
    if (target == "lab") {
      cv::cvtColor(to_bgr(source), converted, cv::COLOR_BGR2Lab);
    } else {
      cv::cvtColor(to_bgr(source), converted, cv::COLOR_BGR2HSV);
    }
    return image_only(converted);
  }

  if (operation == "compare") {
    cv::Mat baseline = to_bgr(original);
    cv::Mat candidate = to_bgr(source);
    if (baseline.size() != candidate.size()) {
      cv::resize(baseline, baseline, candidate.size(), 0, 0, cv::INTER_AREA);
    }
    cv::Mat diff;
    cv::absdiff(baseline, candidate, diff);
    return image_only(diff);
  }

  if (operation == "featureAlign") {
    return image_only(apply_feature_alignment(original, source, params));
  }

  if (operation == "eccAlign") {
    return image_only(apply_ecc_alignment(original, source, params));
  }

  if (operation == "qrScan") {
    return image_only(apply_qr_overlay(source));
  }

  if (operation == "calibrationBoard") {
    return image_only(apply_calibration_board_preview(source, params));
  }

  if (operation == "blobCentroid") {
    return apply_blob_centroid(source, params);
  }

  if (operation == "convexHull") {
    return apply_convex_hull(source, params);
  }

  if (operation == "huMoments") {
    return apply_hu_moments(source, params);
  }

  if (operation == "houghTransform") {
    return apply_hough_transform(source, params);
  }

  if (operation == "inpaint") {
    return apply_inpaint_filter(source, params);
  }

  if (operation == "seamlessClone") {
    return apply_seamless_clone_filter(source, params);
  }

  if (operation == "alphaBlend") {
    return apply_alpha_blend_filter(original, source, params);
  }

  if (operation == "exposureFusion") {
    return apply_exposure_fusion_filter(source, params);
  }

  if (operation == "hdrTonemap") {
    return apply_hdr_tonemap_filter(source, params);
  }

  if (operation == "stylization") {
    return apply_stylization_filter(source, params);
  }

  if (operation == "pencilSketch") {
    return apply_pencil_sketch_filter(source, params);
  }

  if (operation == "visionSampleBoard") {
    return apply_vision_sample_board(source, params);
  }

  if (operation == "perspectiveExtract") {
    return apply_perspective_extract_filter(source, params);
  }

  if (is_dnn_operation(operation)) {
    return apply_dnn_operation(source, operation, params);
  }

  throw std::runtime_error("Unsupported image operation: " + operation);
}

}  // namespace app
