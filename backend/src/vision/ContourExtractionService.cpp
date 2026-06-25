#include "ContourExtractionService.h"

#include "../common/OpenCvUtils.h"
#include "../image/ImageFilters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <string>
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

struct OcrCharacter {
  char text = '?';
  double confidence = 0.0;
  cv::Rect box;
};

struct OcrLine {
  std::string text;
  double confidence = 0.0;
  cv::Rect box;
  std::vector<OcrCharacter> characters;
};

struct OcrReadResult {
  std::string text;
  double confidence = 0.0;
  std::string threshold_mode;
  int component_count = 0;
  std::vector<OcrLine> lines;
};

constexpr int kGlyphWidth = 42;
constexpr int kGlyphHeight = 58;

cv::Mat normalize_glyph_mask(const cv::Mat& mask) {
  std::vector<cv::Point> foreground;
  cv::findNonZero(mask, foreground);
  if (foreground.empty()) {
    return cv::Mat::zeros(kGlyphHeight, kGlyphWidth, CV_8UC1);
  }

  const auto bounds = cv::boundingRect(foreground);
  const cv::Mat cropped = mask(bounds).clone();
  const double scale = std::min(
      static_cast<double>(kGlyphWidth - 8) / static_cast<double>(std::max(1, cropped.cols)),
      static_cast<double>(kGlyphHeight - 8) / static_cast<double>(std::max(1, cropped.rows)));
  const int width = std::clamp(static_cast<int>(std::round(cropped.cols * scale)), 1, kGlyphWidth);
  const int height = std::clamp(static_cast<int>(std::round(cropped.rows * scale)), 1, kGlyphHeight);

  cv::Mat resized;
  cv::resize(cropped, resized, cv::Size(width, height), 0.0, 0.0, cv::INTER_AREA);
  cv::threshold(resized, resized, 128, 255, cv::THRESH_BINARY);

  cv::Mat normalized = cv::Mat::zeros(kGlyphHeight, kGlyphWidth, CV_8UC1);
  const int x = (kGlyphWidth - width) / 2;
  const int y = (kGlyphHeight - height) / 2;
  resized.copyTo(normalized(cv::Rect(x, y, width, height)));
  return normalized;
}

cv::Mat render_ocr_template(char value) {
  cv::Mat canvas = cv::Mat::zeros(128, 96, CV_8UC1);
  const std::string text(1, value);
  int baseline = 0;
  const auto base_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 3, &baseline);
  const double scale = std::min(72.0 / static_cast<double>(std::max(1, base_size.width)), 96.0 / base_size.height);
  const int thickness = value == '1' || value == 'I' ? 3 : 4;
  const auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, scale, thickness, &baseline);
  const cv::Point origin(
      std::max(0, (canvas.cols - text_size.width) / 2),
      std::max(text_size.height, (canvas.rows + text_size.height) / 2 - baseline));
  cv::putText(canvas, text, origin, cv::FONT_HERSHEY_SIMPLEX, scale, cv::Scalar(255), thickness, cv::LINE_AA);
  cv::threshold(canvas, canvas, 96, 255, cv::THRESH_BINARY);
  return normalize_glyph_mask(canvas);
}

const std::vector<std::pair<char, cv::Mat>>& ocr_templates() {
  static const std::vector<std::pair<char, cv::Mat>> templates = [] {
    std::vector<std::pair<char, cv::Mat>> result;
    const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    result.reserve(alphabet.size());
    for (const char value : alphabet) {
      result.emplace_back(value, render_ocr_template(value));
    }
    return result;
  }();
  return templates;
}

double glyph_similarity(const cv::Mat& left, const cv::Mat& right) {
  cv::Mat intersection;
  cv::Mat union_mask;
  cv::bitwise_and(left, right, intersection);
  cv::bitwise_or(left, right, union_mask);
  const double union_count = static_cast<double>(cv::countNonZero(union_mask));
  const double iou = union_count > 0.0 ? static_cast<double>(cv::countNonZero(intersection)) / union_count : 0.0;

  cv::Mat left_float;
  cv::Mat right_float;
  left.convertTo(left_float, CV_32F, 1.0 / 255.0);
  right.convertTo(right_float, CV_32F, 1.0 / 255.0);
  const double max_distance = std::sqrt(static_cast<double>(left.total()));
  const double distance_score = max_distance > 0.0 ? 1.0 - std::min(1.0, cv::norm(left_float - right_float) / max_distance)
                                                   : 0.0;
  return std::clamp(iou * 0.65 + distance_score * 0.35, 0.0, 1.0);
}

OcrCharacter recognize_character(const cv::Mat& component_mask, const cv::Rect& box) {
  const auto glyph = normalize_glyph_mask(component_mask);
  OcrCharacter best;
  best.box = box;
  for (const auto& [value, template_mask] : ocr_templates()) {
    const double score = glyph_similarity(glyph, template_mask);
    if (score > best.confidence) {
      best.text = value;
      best.confidence = score;
    }
  }
  if (best.confidence < 0.28) {
    best.text = '?';
  }
  return best;
}

cv::Mat threshold_for_ocr(const cv::Mat& source, bool inverse) {
  cv::Mat gray = to_gray(source);
  const double scale = std::clamp(900.0 / static_cast<double>(std::max(gray.cols, gray.rows)), 1.0, 3.0);
  if (scale > 1.01) {
    cv::resize(gray, gray, cv::Size(), scale, scale, cv::INTER_CUBIC);
  }
  cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0.0);

  cv::Mat binary;
  cv::threshold(gray, binary, 0, 255, cv::THRESH_OTSU | (inverse ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY));
  cv::morphologyEx(binary, binary, cv::MORPH_OPEN, cv::Mat::ones(2, 2, CV_8U), cv::Point(-1, -1), 1);
  return binary;
}

std::vector<cv::Rect> text_component_boxes(const cv::Mat& binary) {
  cv::Mat labels;
  cv::Mat stats;
  cv::Mat centroids;
  const int count = cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);
  std::vector<cv::Rect> boxes;
  const int image_area = std::max(1, binary.cols * binary.rows);
  const int min_area = std::max(10, image_area / 9000);
  const int max_area = std::max(48, image_area / 4);

  for (int label = 1; label < count; ++label) {
    const int x = stats.at<int>(label, cv::CC_STAT_LEFT);
    const int y = stats.at<int>(label, cv::CC_STAT_TOP);
    const int width = stats.at<int>(label, cv::CC_STAT_WIDTH);
    const int height = stats.at<int>(label, cv::CC_STAT_HEIGHT);
    const int area = stats.at<int>(label, cv::CC_STAT_AREA);
    const double aspect = static_cast<double>(width) / static_cast<double>(std::max(1, height));
    if (area < min_area || area > max_area || width < 3 || height < 8 || aspect < 0.05 || aspect > 2.8) {
      continue;
    }
    if (height > static_cast<int>(binary.rows * 0.85) || width > static_cast<int>(binary.cols * 0.9)) {
      continue;
    }
    boxes.emplace_back(x, y, width, height);
  }

  std::sort(boxes.begin(), boxes.end(), [](const cv::Rect& left, const cv::Rect& right) {
    if (std::abs((left.y + left.height / 2) - (right.y + right.height / 2)) > std::max(left.height, right.height)) {
      return left.y < right.y;
    }
    return left.x < right.x;
  });
  if (boxes.size() > 80) {
    boxes.resize(80);
  }
  return boxes;
}

std::vector<std::vector<cv::Rect>> group_text_lines(std::vector<cv::Rect> boxes) {
  std::sort(boxes.begin(), boxes.end(), [](const cv::Rect& left, const cv::Rect& right) {
    return (left.y + left.height / 2) < (right.y + right.height / 2);
  });

  std::vector<std::vector<cv::Rect>> lines;
  std::vector<double> centers;
  for (const auto& box : boxes) {
    const double center_y = box.y + box.height * 0.5;
    bool added = false;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      const double threshold = std::max(10.0, box.height * 0.62);
      if (std::abs(center_y - centers[index]) <= threshold) {
        lines[index].push_back(box);
        centers[index] = (centers[index] * static_cast<double>(lines[index].size() - 1) + center_y) /
                         static_cast<double>(lines[index].size());
        added = true;
        break;
      }
    }
    if (!added) {
      lines.push_back({box});
      centers.push_back(center_y);
    }
  }

  std::sort(lines.begin(), lines.end(), [](const auto& left, const auto& right) {
    const auto left_min = std::min_element(left.begin(), left.end(), [](const cv::Rect& a, const cv::Rect& b) {
      return a.y < b.y;
    });
    const auto right_min = std::min_element(right.begin(), right.end(), [](const cv::Rect& a, const cv::Rect& b) {
      return a.y < b.y;
    });
    return left_min->y < right_min->y;
  });
  for (auto& line : lines) {
    std::sort(line.begin(), line.end(), [](const cv::Rect& left, const cv::Rect& right) {
      return left.x < right.x;
    });
  }
  return lines;
}

OcrReadResult recognize_ocr_mask(const cv::Mat& source, bool inverse) {
  const cv::Mat binary = threshold_for_ocr(source, inverse);
  const auto boxes = text_component_boxes(binary);
  const auto grouped_lines = group_text_lines(boxes);

  OcrReadResult result;
  result.threshold_mode = inverse ? "inverse-otsu" : "binary-otsu";
  result.component_count = static_cast<int>(boxes.size());

  std::vector<std::string> line_texts;
  std::vector<double> confidences;
  for (const auto& line_boxes : grouped_lines) {
    if (line_boxes.empty()) {
      continue;
    }

    OcrLine line;
    line.box = line_boxes.front();
    std::vector<int> widths;
    widths.reserve(line_boxes.size());
    for (const auto& box : line_boxes) {
      line.box |= box;
      widths.push_back(box.width);
    }
    std::sort(widths.begin(), widths.end());
    const int median_width = widths[widths.size() / 2];

    int previous_right = -1;
    for (const auto& box : line_boxes) {
      if (previous_right >= 0 && box.x - previous_right > std::max(12, median_width)) {
        line.text.push_back(' ');
      }
      const cv::Mat glyph = binary(box).clone();
      auto character = recognize_character(glyph, box);
      line.text.push_back(character.text);
      line.characters.push_back(character);
      confidences.push_back(character.confidence);
      previous_right = box.x + box.width;
    }

    if (!line.characters.empty()) {
      const double total = std::accumulate(
          line.characters.begin(), line.characters.end(), 0.0, [](double sum, const OcrCharacter& character) {
            return sum + character.confidence;
          });
      line.confidence = total / static_cast<double>(line.characters.size());
      line_texts.push_back(line.text);
      result.lines.push_back(line);
    }
  }

  for (std::size_t index = 0; index < line_texts.size(); ++index) {
    if (index > 0) {
      result.text.push_back('\n');
    }
    result.text += line_texts[index];
  }
  result.confidence = confidences.empty()
                          ? 0.0
                          : std::accumulate(confidences.begin(), confidences.end(), 0.0) /
                                static_cast<double>(confidences.size());
  return result;
}

nlohmann::json character_to_json(const OcrCharacter& character) {
  return {
      {"text", std::string(1, character.text)},
      {"confidence", character.confidence},
      {"boundingBox", rect_to_json(character.box)},
  };
}

nlohmann::json line_to_json(const OcrLine& line) {
  auto characters = nlohmann::json::array();
  for (const auto& character : line.characters) {
    characters.push_back(character_to_json(character));
  }
  return {
      {"text", line.text},
      {"confidence", line.confidence},
      {"boundingBox", rect_to_json(line.box)},
      {"characters", characters},
  };
}

nlohmann::json ocr_result_to_json(
    const OcrReadResult& result,
    const cv::Size& image_size,
    const std::string& image_result_id,
    const nlohmann::json& candidate) {
  auto lines = nlohmann::json::array();
  for (const auto& line : result.lines) {
    lines.push_back(line_to_json(line));
  }
  return {
      {"sourceResultId", image_result_id},
      {"candidateId", candidate.value("candidateId", std::string{"manual"})},
      {"text", result.text},
      {"confidence", result.confidence},
      {"lineCount", result.lines.size()},
      {"componentCount", result.component_count},
      {"imageSize", {{"width", image_size.width}, {"height", image_size.height}}},
      {"method",
       {{"engine", "opencv-template-ocr"},
        {"preprocessing", result.threshold_mode},
        {"alphabet", "A-Z 0-9"},
        {"note", "OpenCV-only template OCR for high-contrast printed text."}}},
      {"lines", lines},
  };
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

nlohmann::json ContourExtractionService::recognize_candidate_text(
    const std::string& image_result_id, const nlohmann::json& candidate) const {
  const auto extracted = preview_candidate(image_result_id, candidate);
  if (extracted.empty()) {
    throw std::runtime_error("Selected contour preview was empty.");
  }

  const auto binary_result = recognize_ocr_mask(extracted, false);
  const auto inverse_result = recognize_ocr_mask(extracted, true);
  const auto result_score = [](const OcrReadResult& result) {
    const int char_count = static_cast<int>(std::count_if(result.text.begin(), result.text.end(), [](char value) {
      return value != ' ' && value != '\n';
    }));
    const double noise_penalty = result.component_count > 48 ? static_cast<double>(result.component_count - 48) * 0.025
                                                             : 0.0;
    return result.confidence + std::min(2.0, static_cast<double>(char_count) * 0.09) - noise_penalty;
  };

  const auto& selected = result_score(inverse_result) > result_score(binary_result) ? inverse_result : binary_result;
  return ocr_result_to_json(selected, extracted.size(), image_result_id, candidate);
}

nlohmann::json
ContourExtractionService::extract_candidate(const std::string& image_result_id, const nlohmann::json& candidate) const {
  return image_store_.process(image_result_id, "perspectiveExtract", make_extract_params(candidate));
}

}  // namespace app
