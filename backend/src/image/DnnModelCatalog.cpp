#include "DnnModelCatalog.h"

#include "../common/StringUtils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

namespace app {

namespace {

struct CatalogAsset {
  std::string relative_path;
  std::string url;
  std::string kind;
  std::uintmax_t size_bytes = 0;
};

struct CatalogPackage {
  std::string id;
  std::string label;
  std::string operation;
  std::string description;
  std::string source;
  std::string source_url;
  std::string license_note;
  nlohmann::json params;
  std::vector<CatalogAsset> assets;
};

class WinHttpHandle {
public:
  explicit WinHttpHandle(HINTERNET handle = nullptr)
      : handle_(handle) {}
  ~WinHttpHandle() {
    if (handle_) {
      WinHttpCloseHandle(handle_);
    }
  }

  WinHttpHandle(const WinHttpHandle&) = delete;
  WinHttpHandle& operator=(const WinHttpHandle&) = delete;

  HINTERNET get() const {
    return handle_;
  }

private:
  HINTERNET handle_ = nullptr;
};

std::wstring widen_utf8(std::string_view value) {
  if (value.empty()) {
    return {};
  }

  const int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  if (length <= 0) {
    throw std::runtime_error("Could not convert URL to UTF-16.");
  }

  std::wstring output(static_cast<std::size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), output.data(), length);
  return output;
}

std::filesystem::path dnn_root() {
  const auto root = std::filesystem::current_path() / "models" / "dnn";
  std::filesystem::create_directories(root);
  return std::filesystem::weakly_canonical(root);
}

bool path_has_parent_reference(const std::filesystem::path& path) {
  return std::any_of(path.begin(), path.end(), [](const auto& part) { return part == ".."; });
}

std::filesystem::path resolve_catalog_target(const std::string& relative_path) {
  const auto relative = path_from_utf8(relative_path);
  if (relative.is_absolute() || path_has_parent_reference(relative)) {
    throw std::runtime_error("Catalog asset path must be relative inside models/dnn.");
  }

  const auto root = dnn_root();
  const auto target = std::filesystem::weakly_canonical(root / relative);
  std::error_code error;
  const auto contained = std::filesystem::relative(target, root, error);
  if (error || contained.empty() || *contained.begin() == "..") {
    throw std::runtime_error("Catalog asset path must stay inside models/dnn.");
  }
  return target;
}

bool asset_exists(const CatalogAsset& asset) {
  const auto target = resolve_catalog_target(asset.relative_path);
  if (!std::filesystem::exists(target) || !std::filesystem::is_regular_file(target)) {
    return false;
  }
  return asset.size_bytes == 0 || std::filesystem::file_size(target) == asset.size_bytes;
}

std::vector<CatalogPackage> catalog() {
  return {
      {
          "opencv-face-ssd",
          "OpenCV Face SSD",
          "dnnFaceDetection",
          "ResNet-10 SSD face detector from the OpenCV DNN samples.",
          "OpenCV samples and opencv_3rdparty",
          "https://github.com/opencv/opencv/blob/4.x/samples/dnn/models.yml",
          "Check the upstream OpenCV/opencv_3rdparty license terms before redistribution.",
          {{"modelPath", "face/opencv_face_detector.caffemodel"},
           {"configPath", "face/opencv_face_detector.prototxt"},
           {"inputWidth", 300},
           {"inputHeight", 300},
           {"confidenceThreshold", 0.5},
           {"scale", 1},
           {"meanB", 104},
           {"meanG", 177},
           {"meanR", 123},
           {"swapRB", 0}},
          {
              {"face/opencv_face_detector.prototxt",
               "https://raw.githubusercontent.com/opencv/opencv/4.x/samples/dnn/face_detector/deploy.prototxt",
               "config",
               0},
              {"face/opencv_face_detector.caffemodel",
               "https://github.com/opencv/opencv_3rdparty/raw/dnn_samples_face_detector_20170830/"
               "res10_300x300_ssd_iter_140000.caffemodel",
               "model",
               10666211},
          },
      },
      {
          "yolo11n-coco",
          "YOLO11n COCO ONNX",
          "dnnYoloDetection",
          "Small Ultralytics COCO detector exported to ONNX.",
          "Ultralytics assets and OpenCV COCO labels",
          "https://github.com/ultralytics/assets/releases",
          "Check Ultralytics and COCO label source license terms before redistribution.",
          {{"modelPath", "yolo/yolo11n.onnx"},
           {"labelsPath", "yolo/coco.names"},
           {"inputWidth", 640},
           {"inputHeight", 640},
           {"confidenceThreshold", 0.35},
           {"nmsThreshold", 0.45},
           {"scale", 0.0039215686},
           {"meanB", 0},
           {"meanG", 0},
           {"meanR", 0},
           {"swapRB", 1}},
          {
              {"yolo/yolo11n.onnx",
               "https://github.com/ultralytics/assets/releases/download/v8.3.0/yolo11n.onnx",
               "model",
               10930182},
              {"yolo/coco.names",
               "https://raw.githubusercontent.com/opencv/opencv/4.x/samples/data/dnn/"
               "object_detection_classes_coco.txt",
               "labels",
               0},
          },
      },
      {
          "east-text-detector",
          "EAST Text Detector",
          "dnnTextDetection",
          "Frozen EAST scene text detector used by OpenCV text-detection samples.",
          "EAST model mirror and OpenCV sample documentation",
          "https://github.com/opencv/opencv/blob/4.x/samples/dnn/text_detection.py",
          "Check the upstream EAST model terms before redistribution.",
          {{"modelPath", "text/frozen_east_text_detection.pb"},
           {"inputWidth", 320},
           {"inputHeight", 320},
           {"confidenceThreshold", 0.5}},
          {
              {"text/frozen_east_text_detection.pb",
               "https://raw.githubusercontent.com/oyyd/frozen_east_text_detection.pb/master/"
               "frozen_east_text_detection.pb",
               "model",
               96662756},
          },
      },
      {
          "openpose-coco",
          "OpenPose COCO Pose",
          "dnnPoseEstimation",
          "OpenPose COCO pose-estimation Caffe model for OpenCV DNN.",
          "OpenPose model mirrors and OpenCV extra test data",
          "https://github.com/CMU-Perceptual-Computing-Lab/openpose/blob/master/models/getModels.sh",
          "Check OpenPose and model mirror terms before redistribution.",
          {{"modelPath", "pose/pose_iter_440000.caffemodel"},
           {"configPath", "pose/openpose_pose_coco.prototxt"},
           {"inputWidth", 368},
           {"inputHeight", 368},
           {"confidenceThreshold", 0.2},
           {"scale", 0.0039215686},
           {"meanB", 0},
           {"meanG", 0},
           {"meanR", 0},
           {"swapRB", 1}},
          {
              {"pose/openpose_pose_coco.prototxt",
               "https://raw.githubusercontent.com/opencv/opencv_extra/4.x/testdata/dnn/openpose_pose_coco.prototxt",
               "config",
               0},
              {"pose/pose_iter_440000.caffemodel",
               "https://media.githubusercontent.com/media/foss-for-synopsys-dwc-arc-processors/"
               "synopsys-caffe-models/master/caffe_models/openpose/caffe_model/pose_iter_440000.caffemodel",
               "model",
               209274056},
          },
      },
      {
          "mask-rcnn-coco",
          "Mask R-CNN COCO",
          "dnnMaskRcnn",
          "TensorFlow Mask R-CNN COCO graph and OpenCV-compatible config.",
          "TensorFlow/OpenCV Mask R-CNN public mirrors",
          "https://learnopencv.com/"
          "deep-learning-based-object-detection-and-instance-segmentation-using-mask-rcnn-in-opencv-python-c/",
          "Check TensorFlow model zoo and mirror terms before redistribution.",
          {{"modelPath", "mask-rcnn/frozen_inference_graph.pb"},
           {"configPath", "mask-rcnn/mask_rcnn_inception_v2_coco_2018_01_28.pbtxt"},
           {"labelsPath", "mask-rcnn/coco.names"},
           {"inputWidth", 800},
           {"inputHeight", 800},
           {"confidenceThreshold", 0.5},
           {"maskThreshold", 0.3},
           {"scale", 0.0039215686},
           {"meanB", 0},
           {"meanG", 0},
           {"meanR", 0},
           {"swapRB", 1}},
          {
              {"mask-rcnn/frozen_inference_graph.pb",
               "https://raw.githubusercontent.com/methylDragon/opencv-python-reference/master/Resources/Models/"
               "mask-rcnn-coco/frozen_inference_graph.pb",
               "model",
               67138064},
              {"mask-rcnn/mask_rcnn_inception_v2_coco_2018_01_28.pbtxt",
               "https://raw.githubusercontent.com/methylDragon/opencv-python-reference/master/Resources/Models/"
               "mask-rcnn-coco/mask_rcnn_inception_v2_coco_2018_01_28.pbtxt",
               "config",
               0},
              {"mask-rcnn/coco.names",
               "https://raw.githubusercontent.com/methylDragon/opencv-python-reference/master/Resources/Models/"
               "mask-rcnn-coco/object_detection_classes_coco.txt",
               "labels",
               0},
          },
      },
  };
}

std::uintmax_t package_size(const CatalogPackage& package) {
  return std::accumulate(
      package.assets.begin(), package.assets.end(), std::uintmax_t{0}, [](auto total, const auto& asset) {
        return total + asset.size_bytes;
      });
}

nlohmann::json asset_to_json(const CatalogAsset& asset) {
  const bool exists = asset_exists(asset);
  return {
      {"relativePath", asset.relative_path},
      {"url", asset.url},
      {"kind", asset.kind},
      {"sizeBytes", asset.size_bytes},
      {"downloaded", exists},
  };
}

nlohmann::json package_to_json(const CatalogPackage& package) {
  auto assets = nlohmann::json::array();
  bool downloaded = true;
  for (const auto& asset : package.assets) {
    const auto asset_json = asset_to_json(asset);
    downloaded = downloaded && asset_json.value("downloaded", false);
    assets.push_back(asset_json);
  }

  return {
      {"id", package.id},
      {"label", package.label},
      {"operation", package.operation},
      {"description", package.description},
      {"source", package.source},
      {"sourceUrl", package.source_url},
      {"licenseNote", package.license_note},
      {"params", package.params},
      {"totalBytes", package_size(package)},
      {"downloaded", downloaded},
      {"assets", assets},
  };
}

const CatalogPackage& find_package(const std::string& package_id) {
  static const auto packages = catalog();
  const auto found =
      std::find_if(packages.begin(), packages.end(), [&](const auto& item) { return item.id == package_id; });
  if (found == packages.end()) {
    throw std::runtime_error("Unknown DNN model package: " + package_id);
  }
  return *found;
}

DWORD query_status_code(HINTERNET request) {
  DWORD status_code = 0;
  DWORD size = sizeof(status_code);
  WinHttpQueryHeaders(
      request,
      WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
      WINHTTP_HEADER_NAME_BY_INDEX,
      &status_code,
      &size,
      WINHTTP_NO_HEADER_INDEX);
  return status_code;
}

std::uintmax_t download_url_to_file(
    const std::string& url,
    const std::filesystem::path& target,
    std::uintmax_t expected_size,
    const std::function<bool()>& should_cancel,
    const std::function<void(std::uintmax_t)>& report_bytes) {
  const auto wide_url = widen_utf8(url);
  URL_COMPONENTS components{};
  components.dwStructSize = sizeof(components);
  components.dwSchemeLength = static_cast<DWORD>(-1);
  components.dwHostNameLength = static_cast<DWORD>(-1);
  components.dwUrlPathLength = static_cast<DWORD>(-1);
  components.dwExtraInfoLength = static_cast<DWORD>(-1);
  if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &components)) {
    throw std::runtime_error("Could not parse model download URL.");
  }

  const std::wstring host(components.lpszHostName, components.dwHostNameLength);
  std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
  if (components.dwExtraInfoLength > 0) {
    path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
  }
  const bool secure = components.nScheme == INTERNET_SCHEME_HTTPS;

  WinHttpHandle session(WinHttpOpen(
      L"ReactMUIOpenCV/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
  if (!session.get()) {
    throw std::runtime_error("WinHTTP session could not be opened.");
  }

  DWORD redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
  WinHttpSetOption(session.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirect_policy, sizeof(redirect_policy));

  WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), components.nPort, 0));
  if (!connection.get()) {
    throw std::runtime_error("WinHTTP connection could not be opened.");
  }

  WinHttpHandle request(WinHttpOpenRequest(
      connection.get(),
      L"GET",
      path.c_str(),
      nullptr,
      WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES,
      secure ? WINHTTP_FLAG_SECURE : 0));
  if (!request.get()) {
    throw std::runtime_error("WinHTTP request could not be opened.");
  }

  if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
      !WinHttpReceiveResponse(request.get(), nullptr)) {
    throw std::runtime_error("Model download request failed.");
  }

  const DWORD status_code = query_status_code(request.get());
  if (status_code < 200 || status_code >= 300) {
    throw std::runtime_error("Model download failed with HTTP status " + std::to_string(status_code) + ".");
  }

  std::filesystem::create_directories(target.parent_path());
  const auto temporary = target.string() + ".download";
  std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Could not create model download file.");
  }

  std::uintmax_t written = 0;
  while (true) {
    if (should_cancel()) {
      output.close();
      std::filesystem::remove(temporary);
      throw std::runtime_error("Model download was cancelled.");
    }

    DWORD available = 0;
    if (!WinHttpQueryDataAvailable(request.get(), &available)) {
      throw std::runtime_error("Model download read failed.");
    }
    if (available == 0) {
      break;
    }

    std::vector<char> buffer(available);
    DWORD read = 0;
    if (!WinHttpReadData(request.get(), buffer.data(), available, &read)) {
      throw std::runtime_error("Model download read failed.");
    }
    output.write(buffer.data(), read);
    written += read;
    report_bytes(written);
  }
  output.close();

  if (expected_size > 0 && written != expected_size) {
    std::filesystem::remove(temporary);
    throw std::runtime_error("Downloaded model size did not match the catalog metadata.");
  }

  std::filesystem::remove(target);
  std::filesystem::rename(temporary, target);
  return written;
}

}  // namespace

nlohmann::json list_dnn_model_packages() {
  auto packages = nlohmann::json::array();
  for (const auto& package : catalog()) {
    packages.push_back(package_to_json(package));
  }
  return {{"root", "models/dnn"}, {"packages", packages}};
}

nlohmann::json download_dnn_model_package(
    const std::string& package_id,
    const std::function<bool()>& should_cancel,
    const std::function<void(int, const std::string&)>& report_progress) {
  const auto& package = find_package(package_id);
  const auto total = std::max<std::uintmax_t>(1, package_size(package));
  std::uintmax_t completed = 0;

  for (const auto& asset : package.assets) {
    const auto target = resolve_catalog_target(asset.relative_path);
    if (asset_exists(asset)) {
      completed += asset.size_bytes;
      report_progress(static_cast<int>((completed * 100) / total), "Already available: " + asset.relative_path);
      continue;
    }

    report_progress(static_cast<int>((completed * 100) / total), "Downloading " + asset.relative_path);
    const auto written =
        download_url_to_file(asset.url, target, asset.size_bytes, should_cancel, [&](std::uintmax_t asset_written) {
          const auto bounded = asset.size_bytes == 0 ? asset_written : std::min(asset_written, asset.size_bytes);
          const auto current = completed + bounded;
          report_progress(static_cast<int>((current * 100) / total), "Downloading " + asset.relative_path);
        });
    completed += asset.size_bytes == 0 ? written : asset.size_bytes;
  }

  report_progress(100, "Downloaded " + package.label);
  return package_to_json(package);
}

}  // namespace app
