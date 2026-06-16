#include "ImageDnnFilters.h"

#include "../common/JsonUtils.h"
#include "../common/OpenCvUtils.h"
#include "../common/StringUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace app {

namespace {

constexpr std::array<std::string_view, 5> kDnnOperations = {
    "dnnFaceDetection", "dnnYoloDetection", "dnnTextDetection", "dnnPoseEstimation", "dnnMaskRcnn"};

std::filesystem::path dnn_model_root() {
  const auto root = std::filesystem::current_path() / "models" / "dnn";
  std::error_code error;
  std::filesystem::create_directories(root, error);
  return std::filesystem::weakly_canonical(root);
}

bool path_has_parent_reference(const std::filesystem::path& path) {
  return std::any_of(path.begin(), path.end(), [](const auto& part) { return part == ".."; });
}

std::filesystem::path
resolve_model_asset(const nlohmann::json& params, const char* key, bool required, const std::string& label) {
  const auto value = json_value_or<std::string>(params, key, {});
  if (value.empty()) {
    if (required) {
      throw std::runtime_error(
          label + " is required. Place the file under models/dnn and set the relative path in Image Lab.");
    }
    return {};
  }

  const auto relative = path_from_utf8(value);
  if (relative.is_absolute() || path_has_parent_reference(relative)) {
    throw std::runtime_error(label + " must be a relative path inside models/dnn.");
  }

  const auto root = dnn_model_root();
  const auto candidate = std::filesystem::weakly_canonical(root / relative);
  std::error_code error;
  const auto contained = std::filesystem::relative(candidate, root, error);
  if (error || contained.empty() || *contained.begin() == "..") {
    throw std::runtime_error(label + " must stay inside models/dnn.");
  }

  if (!std::filesystem::exists(candidate) || std::filesystem::is_directory(candidate)) {
    throw std::runtime_error(
        label + " was not found in models/dnn: " + value +
        ". Add the model asset, then use the relative path shown in the Image Lab DNN card.");
  }

  return candidate;
}

cv::Scalar color_for_index(int index) {
  static const std::array<cv::Scalar, 8> colors = {
      cv::Scalar(37, 99, 235),
      cv::Scalar(15, 118, 110),
      cv::Scalar(194, 65, 12),
      cv::Scalar(147, 51, 234),
      cv::Scalar(220, 38, 38),
      cv::Scalar(8, 145, 178),
      cv::Scalar(234, 179, 8),
      cv::Scalar(22, 163, 74),
  };
  return colors[static_cast<std::size_t>(std::abs(index)) % colors.size()];
}

double dnn_double(const nlohmann::json& params, const char* key, double fallback, double min, double max) {
  return std::clamp(json_value_or<double>(params, key, fallback), min, max);
}

int dnn_int(const nlohmann::json& params, const char* key, int fallback, int min, int max) {
  return std::clamp(json_value_or<int>(params, key, fallback), min, max);
}

bool dnn_bool(const nlohmann::json& params, const char* key, bool fallback) {
  if (!params.is_object() || !params.contains(key)) {
    return fallback;
  }

  const auto& value = params.at(key);
  if (value.is_boolean()) {
    return value.get<bool>();
  }
  if (value.is_number_integer()) {
    return value.get<int>() != 0;
  }
  if (value.is_string()) {
    const auto text = lowercase_copy(value.get<std::string>());
    return text == "true" || text == "1" || text == "yes";
  }
  return fallback;
}

cv::dnn::Net read_guarded_net(const nlohmann::json& params, bool config_required = false) {
  const auto model = resolve_model_asset(params, "modelPath", true, "DNN modelPath");
  const auto config = resolve_model_asset(params, "configPath", config_required, "DNN configPath");
  cv::dnn::Net net;
  try {
    net = config.empty() ? cv::dnn::readNet(model.string()) : cv::dnn::readNet(model.string(), config.string());
  } catch (const cv::Exception& error) {
    throw std::runtime_error("OpenCV DNN failed to load the selected model: " + std::string(error.what()));
  }

  if (net.empty()) {
    throw std::runtime_error("OpenCV DNN loaded an empty model. Check the model/config pairing in models/dnn.");
  }
  return net;
}

std::vector<std::string> read_labels(const nlohmann::json& params) {
  const auto path = resolve_model_asset(params, "labelsPath", false, "DNN labelsPath");
  if (path.empty()) {
    return {};
  }

  std::ifstream file(path);
  std::vector<std::string> labels;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty()) {
      labels.push_back(line);
    }
  }
  return labels;
}

cv::Mat make_blob(const cv::Mat& source, const nlohmann::json& params, int default_width, int default_height) {
  const int width = dnn_int(params, "inputWidth", default_width, 32, 4096);
  const int height = dnn_int(params, "inputHeight", default_height, 32, 4096);
  const double scale = dnn_double(params, "scale", 1.0 / 255.0, 0.000001, 255.0);
  const bool swap_rb = dnn_bool(params, "swapRB", true);
  const cv::Scalar mean(
      dnn_double(params, "meanB", 0.0, -255.0, 255.0),
      dnn_double(params, "meanG", 0.0, -255.0, 255.0),
      dnn_double(params, "meanR", 0.0, -255.0, 255.0));
  return cv::dnn::blobFromImage(to_bgr(source), scale, cv::Size(width, height), mean, swap_rb, false);
}

nlohmann::json base_dnn_metadata(const std::string& operation) {
  return {{"dnn", {{"operation", operation}, {"modelRoot", "models/dnn"}}}};
}

void draw_label(cv::Mat& output, const std::string& label, const cv::Point& origin, const cv::Scalar& color) {
  int baseline = 0;
  const auto size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
  const int x = std::clamp(origin.x, 0, std::max(0, output.cols - size.width - 8));
  const int y = std::clamp(origin.y, size.height + 8, std::max(size.height + 8, output.rows - 4));
  cv::rectangle(output, cv::Rect(x, y - size.height - 7, size.width + 8, size.height + 10), color, cv::FILLED);
  cv::putText(
      output, label, cv::Point(x + 4, y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
}

nlohmann::json point_to_dnn_json(const cv::Point& point) {
  return {{"x", point.x}, {"y", point.y}};
}

ImageOperationResult apply_face_detection(const cv::Mat& source, const nlohmann::json& params) {
  auto net = read_guarded_net(params);
  const int width = dnn_int(params, "inputWidth", 300, 32, 4096);
  const int height = dnn_int(params, "inputHeight", 300, 32, 4096);
  const double scale = dnn_double(params, "scale", 1.0, 0.000001, 255.0);
  const double confidence_threshold = dnn_double(params, "confidenceThreshold", 0.5, 0.01, 1.0);
  const cv::Scalar mean(
      dnn_double(params, "meanB", 104.0, -255.0, 255.0),
      dnn_double(params, "meanG", 177.0, -255.0, 255.0),
      dnn_double(params, "meanR", 123.0, -255.0, 255.0));

  net.setInput(cv::dnn::blobFromImage(to_bgr(source), scale, cv::Size(width, height), mean, false, false));
  cv::Mat detections = net.forward();
  cv::Mat output = to_bgr(source);
  auto items = nlohmann::json::array();

  if (detections.dims == 4 && detections.size[3] >= 7) {
    const int count = detections.size[2];
    cv::Mat rows(count, detections.size[3], CV_32F, detections.ptr<float>());
    for (int i = 0; i < rows.rows; ++i) {
      const float* row = rows.ptr<float>(i);
      const double confidence = row[2];
      if (confidence < confidence_threshold) {
        continue;
      }

      const int x1 = std::clamp(static_cast<int>(row[3] * output.cols), 0, output.cols - 1);
      const int y1 = std::clamp(static_cast<int>(row[4] * output.rows), 0, output.rows - 1);
      const int x2 = std::clamp(static_cast<int>(row[5] * output.cols), 0, output.cols - 1);
      const int y2 = std::clamp(static_cast<int>(row[6] * output.rows), 0, output.rows - 1);
      const cv::Rect box(cv::Point(std::min(x1, x2), std::min(y1, y2)), cv::Point(std::max(x1, x2), std::max(y1, y2)));
      if (box.area() <= 0) {
        continue;
      }

      cv::rectangle(output, box, cv::Scalar(37, 99, 235), 2, cv::LINE_AA);
      draw_label(
          output, "face " + std::to_string(static_cast<int>(confidence * 100)) + "%", box.tl(), color_for_index(i));
      items.push_back({{"confidence", confidence}, {"boundingBox", rect_to_json(box)}});
    }
  } else {
    throw std::runtime_error("Face detection expected an SSD-style output with shape [1, 1, N, 7].");
  }

  auto metadata = base_dnn_metadata("dnnFaceDetection");
  metadata["dnn"]["detectionCount"] = items.size();
  metadata["dnn"]["detections"] = items;
  return {output, metadata};
}

cv::Mat detection_rows(const cv::Mat& output) {
  if (output.dims == 2) {
    return output;
  }
  if (output.dims == 3) {
    cv::Mat rows(output.size[1], output.size[2], CV_32F, const_cast<float*>(output.ptr<float>()));
    if (rows.rows < rows.cols && rows.cols > 128) {
      cv::Mat transposed;
      cv::transpose(rows, transposed);
      return transposed;
    }
    return rows;
  }
  return output.reshape(
      1, static_cast<int>(output.total() / static_cast<std::size_t>(std::max(1, output.size[output.dims - 1]))));
}

ImageOperationResult apply_yolo_detection(const cv::Mat& source, const nlohmann::json& params) {
  auto net = read_guarded_net(params);
  const auto labels = read_labels(params);
  const double confidence_threshold = dnn_double(params, "confidenceThreshold", 0.35, 0.01, 1.0);
  const double nms_threshold = dnn_double(params, "nmsThreshold", 0.45, 0.01, 1.0);
  net.setInput(make_blob(source, params, 640, 640));

  std::vector<cv::Mat> outputs;
  net.forward(outputs, net.getUnconnectedOutLayersNames());

  std::vector<int> class_ids;
  std::vector<float> confidences;
  std::vector<cv::Rect> boxes;
  for (const auto& raw : outputs) {
    const auto rows = detection_rows(raw);
    for (int i = 0; i < rows.rows; ++i) {
      const float* row = rows.ptr<float>(i);
      if (rows.cols < 6) {
        continue;
      }

      const int class_offset = rows.cols > 84 ? 5 : 4;
      const float objectness = class_offset == 5 ? row[4] : 1.0F;
      int class_id = 0;
      float class_score = 0.0F;
      for (int c = class_offset; c < rows.cols; ++c) {
        if (row[c] > class_score) {
          class_score = row[c];
          class_id = c - class_offset;
        }
      }
      const float confidence = objectness * class_score;
      if (confidence < confidence_threshold) {
        continue;
      }

      float cx = row[0];
      float cy = row[1];
      float width = row[2];
      float height = row[3];
      if (std::max({std::abs(cx), std::abs(cy), std::abs(width), std::abs(height)}) <= 2.0F) {
        cx *= static_cast<float>(source.cols);
        width *= static_cast<float>(source.cols);
        cy *= static_cast<float>(source.rows);
        height *= static_cast<float>(source.rows);
      }
      boxes.emplace_back(
          cv::Rect(
              cv::Point(static_cast<int>(cx - width / 2.0F), static_cast<int>(cy - height / 2.0F)),
              cv::Size(static_cast<int>(width), static_cast<int>(height))) &
          cv::Rect(0, 0, source.cols, source.rows));
      class_ids.push_back(class_id);
      confidences.push_back(confidence);
    }
  }

  std::vector<int> keep;
  cv::dnn::NMSBoxes(
      boxes, confidences, static_cast<float>(confidence_threshold), static_cast<float>(nms_threshold), keep);
  cv::Mat output = to_bgr(source);
  auto detections = nlohmann::json::array();
  for (const int index : keep) {
    if (boxes[index].area() <= 0) {
      continue;
    }
    const auto color = color_for_index(class_ids[index]);
    const auto label = class_ids[index] >= 0 && static_cast<std::size_t>(class_ids[index]) < labels.size()
                           ? labels[static_cast<std::size_t>(class_ids[index])]
                           : "class " + std::to_string(class_ids[index]);
    cv::rectangle(output, boxes[index], color, 2, cv::LINE_AA);
    draw_label(output, label, boxes[index].tl(), color);
    detections.push_back(
        {{"classId", class_ids[index]},
         {"label", label},
         {"confidence", confidences[index]},
         {"boundingBox", rect_to_json(boxes[index])}});
  }

  auto metadata = base_dnn_metadata("dnnYoloDetection");
  metadata["dnn"]["detectionCount"] = detections.size();
  metadata["dnn"]["detections"] = detections;
  return {output, metadata};
}

std::vector<cv::Rect> decode_east_boxes(
    const cv::Mat& scores, const cv::Mat& geometry, float confidence_threshold, float scale_x, float scale_y) {
  if (scores.dims != 4 || geometry.dims != 4 || geometry.size[1] < 5) {
    throw std::runtime_error("Text detection expected EAST score and geometry outputs.");
  }

  std::vector<cv::Rect> boxes;
  for (int y = 0; y < scores.size[2]; ++y) {
    const float* score_data = scores.ptr<float>(0, 0, y);
    const float* x0_data = geometry.ptr<float>(0, 0, y);
    const float* x1_data = geometry.ptr<float>(0, 1, y);
    const float* x2_data = geometry.ptr<float>(0, 2, y);
    const float* x3_data = geometry.ptr<float>(0, 3, y);
    const float* angle_data = geometry.ptr<float>(0, 4, y);
    for (int x = 0; x < scores.size[3]; ++x) {
      if (score_data[x] < confidence_threshold) {
        continue;
      }
      const float offset_x = static_cast<float>(x * 4);
      const float offset_y = static_cast<float>(y * 4);
      const float angle = angle_data[x];
      const float cosine = std::cos(angle);
      const float sine = std::sin(angle);
      const float height = x0_data[x] + x2_data[x];
      const float width = x1_data[x] + x3_data[x];
      const float end_x = offset_x + cosine * x1_data[x] + sine * x2_data[x];
      const float end_y = offset_y - sine * x1_data[x] + cosine * x2_data[x];
      const float start_x = end_x - width;
      const float start_y = end_y - height;
      boxes.emplace_back(
          static_cast<int>(start_x * scale_x),
          static_cast<int>(start_y * scale_y),
          static_cast<int>(width * scale_x),
          static_cast<int>(height * scale_y));
    }
  }
  return boxes;
}

ImageOperationResult apply_text_detection(const cv::Mat& source, const nlohmann::json& params) {
  auto net = read_guarded_net(params);
  const int width = dnn_int(params, "inputWidth", 320, 32, 4096);
  const int height = dnn_int(params, "inputHeight", 320, 32, 4096);
  const float confidence_threshold = static_cast<float>(dnn_double(params, "confidenceThreshold", 0.5, 0.01, 1.0));
  net.setInput(
      cv::dnn::blobFromImage(
          to_bgr(source), 1.0, cv::Size(width, height), cv::Scalar(123.68, 116.78, 103.94), true, false));

  std::vector<cv::Mat> outputs;
  const std::vector<std::string> output_names = {"feature_fusion/Conv_7/Sigmoid", "feature_fusion/concat_3"};
  net.forward(outputs, output_names);
  if (outputs.size() != 2) {
    throw std::runtime_error("Text detection could not read EAST output layers.");
  }

  const float scale_x = static_cast<float>(source.cols) / static_cast<float>(width);
  const float scale_y = static_cast<float>(source.rows) / static_cast<float>(height);
  auto boxes = decode_east_boxes(outputs[0], outputs[1], confidence_threshold, scale_x, scale_y);
  std::vector<float> confidences(boxes.size(), 1.0F);
  std::vector<int> keep;
  cv::dnn::NMSBoxes(boxes, confidences, confidence_threshold, 0.4F, keep);

  cv::Mat output = to_bgr(source);
  auto detections = nlohmann::json::array();
  for (const int index : keep) {
    const auto box = boxes[index] & cv::Rect(0, 0, output.cols, output.rows);
    if (box.area() <= 0) {
      continue;
    }
    cv::rectangle(output, box, cv::Scalar(15, 118, 110), 2, cv::LINE_AA);
    detections.push_back({{"boundingBox", rect_to_json(box)}});
  }

  auto metadata = base_dnn_metadata("dnnTextDetection");
  metadata["dnn"]["detectionCount"] = detections.size();
  metadata["dnn"]["detections"] = detections;
  return {output, metadata};
}

ImageOperationResult apply_pose_estimation(const cv::Mat& source, const nlohmann::json& params) {
  auto net = read_guarded_net(params);
  const double confidence_threshold = dnn_double(params, "confidenceThreshold", 0.2, 0.01, 1.0);
  net.setInput(make_blob(source, params, 368, 368));
  cv::Mat output_blob = net.forward();
  if (output_blob.dims != 4) {
    throw std::runtime_error("Pose estimation expected a heatmap output with shape [1, parts, H, W].");
  }

  const int parts = output_blob.size[1];
  const int height = output_blob.size[2];
  const int width = output_blob.size[3];
  std::vector<cv::Point> points(parts, cv::Point(-1, -1));
  auto joints = nlohmann::json::array();
  for (int part = 0; part < parts; ++part) {
    cv::Mat heatmap(height, width, CV_32F, output_blob.ptr<float>(0, part));
    double confidence = 0.0;
    cv::Point max_point;
    cv::minMaxLoc(heatmap, nullptr, &confidence, nullptr, &max_point);
    if (confidence < confidence_threshold) {
      continue;
    }
    points[part] = cv::Point(
        static_cast<int>((static_cast<double>(source.cols) * max_point.x) / width),
        static_cast<int>((static_cast<double>(source.rows) * max_point.y) / height));
    joints.push_back({{"index", part}, {"confidence", confidence}, {"point", point_to_dnn_json(points[part])}});
  }

  static constexpr std::array<std::pair<int, int>, 17> kCocoPairs = {
      std::pair{1, 2},
      {1, 5},
      {2, 3},
      {3, 4},
      {5, 6},
      {6, 7},
      {1, 8},
      {8, 9},
      {9, 10},
      {1, 11},
      {11, 12},
      {12, 13},
      {1, 0},
      {0, 14},
      {14, 16},
      {0, 15},
      {15, 17},
  };

  cv::Mat output = to_bgr(source);
  for (const auto& [a, b] : kCocoPairs) {
    if (a < parts && b < parts && points[a].x >= 0 && points[b].x >= 0) {
      cv::line(output, points[a], points[b], cv::Scalar(37, 99, 235), 2, cv::LINE_AA);
    }
  }
  for (int part = 0; part < parts; ++part) {
    if (points[part].x >= 0) {
      cv::circle(output, points[part], 4, color_for_index(part), cv::FILLED, cv::LINE_AA);
    }
  }

  auto metadata = base_dnn_metadata("dnnPoseEstimation");
  metadata["dnn"]["jointCount"] = joints.size();
  metadata["dnn"]["joints"] = joints;
  return {output, metadata};
}

ImageOperationResult apply_mask_rcnn(const cv::Mat& source, const nlohmann::json& params) {
  auto net = read_guarded_net(params);
  const auto labels = read_labels(params);
  const double confidence_threshold = dnn_double(params, "confidenceThreshold", 0.5, 0.01, 1.0);
  net.setInput(make_blob(source, params, 800, 800));
  std::vector<cv::Mat> outputs;
  const std::vector<std::string> output_names = {"detection_out_final", "detection_masks"};
  net.forward(outputs, output_names);
  if (outputs.size() != 2 || outputs[0].dims != 4 || outputs[1].dims != 4) {
    throw std::runtime_error("Mask R-CNN expected detection_out_final and detection_masks outputs.");
  }

  cv::Mat output = to_bgr(source);
  auto detections = nlohmann::json::array();
  const int count = outputs[0].size[2];
  cv::Mat rows(count, outputs[0].size[3], CV_32F, outputs[0].ptr<float>());
  for (int i = 0; i < rows.rows; ++i) {
    const float* row = rows.ptr<float>(i);
    const int class_id = static_cast<int>(row[1]);
    const double confidence = row[2];
    if (confidence < confidence_threshold) {
      continue;
    }

    const int x1 = std::clamp(static_cast<int>(row[3] * source.cols), 0, source.cols - 1);
    const int y1 = std::clamp(static_cast<int>(row[4] * source.rows), 0, source.rows - 1);
    const int x2 = std::clamp(static_cast<int>(row[5] * source.cols), 0, source.cols - 1);
    const int y2 = std::clamp(static_cast<int>(row[6] * source.rows), 0, source.rows - 1);
    const cv::Rect box(cv::Point(std::min(x1, x2), std::min(y1, y2)), cv::Point(std::max(x1, x2), std::max(y1, y2)));
    if (box.area() <= 0 || class_id < 0 || class_id >= outputs[1].size[1]) {
      continue;
    }

    cv::Mat mask(outputs[1].size[2], outputs[1].size[3], CV_32F, outputs[1].ptr<float>(i, class_id));
    cv::resize(mask, mask, box.size());
    cv::Mat binary = mask > static_cast<float>(dnn_double(params, "maskThreshold", 0.3, 0.01, 1.0));
    const auto color = color_for_index(class_id);
    cv::Mat roi = output(box);
    cv::Mat original_roi = roi.clone();
    cv::Mat color_layer(roi.size(), roi.type(), color);
    color_layer.copyTo(roi, binary);
    cv::addWeighted(original_roi, 0.72, roi, 0.28, 0.0, output(box));
    cv::rectangle(output, box, color, 2, cv::LINE_AA);
    const auto label = class_id >= 0 && static_cast<std::size_t>(class_id) < labels.size()
                           ? labels[static_cast<std::size_t>(class_id)]
                           : "class " + std::to_string(class_id);
    draw_label(output, label, box.tl(), color);
    detections.push_back(
        {{"classId", class_id}, {"label", label}, {"confidence", confidence}, {"boundingBox", rect_to_json(box)}});
  }

  auto metadata = base_dnn_metadata("dnnMaskRcnn");
  metadata["dnn"]["detectionCount"] = detections.size();
  metadata["dnn"]["detections"] = detections;
  return {output, metadata};
}

std::string model_kind(const std::filesystem::path& path) {
  const auto extension = lowercase_copy(path.extension().string());
  if (extension == ".names" || extension == ".txt") {
    return "labels";
  }
  if (extension == ".cfg" || extension == ".pbtxt" || extension == ".prototxt") {
    return "config";
  }
  return "model";
}

bool is_listed_model_asset(const std::filesystem::path& path) {
  return has_supported_extension(
      path,
      {".onnx", ".pb", ".pbtxt", ".caffemodel", ".prototxt", ".weights", ".cfg", ".t7", ".net", ".names", ".txt"});
}

}  // namespace

bool is_dnn_operation(const std::string& operation) {
  return std::find(kDnnOperations.begin(), kDnnOperations.end(), operation) != kDnnOperations.end();
}

ImageOperationResult
apply_dnn_operation(const cv::Mat& source, const std::string& operation, const nlohmann::json& params) {
  if (operation == "dnnFaceDetection") {
    return apply_face_detection(source, params);
  }
  if (operation == "dnnYoloDetection") {
    return apply_yolo_detection(source, params);
  }
  if (operation == "dnnTextDetection") {
    return apply_text_detection(source, params);
  }
  if (operation == "dnnPoseEstimation") {
    return apply_pose_estimation(source, params);
  }
  if (operation == "dnnMaskRcnn") {
    return apply_mask_rcnn(source, params);
  }
  throw std::runtime_error("Unsupported DNN image operation: " + operation);
}

nlohmann::json list_dnn_model_assets() {
  const auto root = dnn_model_root();
  std::error_code error;
  std::filesystem::create_directories(root, error);

  auto models = nlohmann::json::array();
  if (std::filesystem::exists(root)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
      if (!entry.is_regular_file() || !is_listed_model_asset(entry.path())) {
        continue;
      }
      const auto relative = std::filesystem::relative(entry.path(), root, error);
      if (error || relative.empty() || path_has_parent_reference(relative)) {
        continue;
      }
      models.push_back(
          {{"relativePath", path_to_utf8(relative)},
           {"name", path_to_utf8(entry.path().filename())},
           {"extension", lowercase_copy(entry.path().extension().string())},
           {"kind", model_kind(entry.path())},
           {"sizeBytes", entry.file_size()}});
    }
  }

  return {{"root", "models/dnn"}, {"models", models}};
}

}  // namespace app
