#include "PerformanceBenchmarkService.h"

#include "../common/OpenCvUtils.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <opencv2/core.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

namespace app {

namespace {

struct BenchmarkMethod {
  std::string name;
  std::string label;
  std::function<cv::Mat(const cv::Mat&)> run;
};

double checksum(const cv::Mat& image) {
  const auto sum = cv::sum(image);
  double total = 0.0;
  for (int index = 0; index < image.channels(); ++index) {
    total += sum[index];
  }
  return total;
}

cv::Mat invert_with_pointer_loop(const cv::Mat& source) {
  cv::Mat output(source.size(), source.type());
  const int channels = source.channels();
  for (int row = 0; row < source.rows; ++row) {
    const auto* input_row = source.ptr<unsigned char>(row);
    auto* output_row = output.ptr<unsigned char>(row);
    for (int col = 0; col < source.cols * channels; ++col) {
      output_row[col] = static_cast<unsigned char>(255 - input_row[col]);
    }
  }
  return output;
}

cv::Mat invert_with_for_each(const cv::Mat& source) {
  cv::Mat output = source.clone();
  cv::Mat bytes = output.reshape(1);
  bytes.forEach<unsigned char>(
      [](unsigned char& value, const int*) { value = static_cast<unsigned char>(255 - value); });
  return output;
}

cv::Mat invert_with_bitwise_not(const cv::Mat& source) {
  cv::Mat output;
  cv::bitwise_not(source, output);
  return output;
}

nlohmann::json measure_method(const BenchmarkMethod& method, const cv::Mat& source, int iterations) {
  method.run(source);

  const auto started = std::chrono::steady_clock::now();
  double last_checksum = 0.0;
  for (int iteration = 0; iteration < iterations; ++iteration) {
    const auto output = method.run(source);
    last_checksum = checksum(output);
  }
  const auto finished = std::chrono::steady_clock::now();
  const auto total_ms = std::chrono::duration<double, std::milli>(finished - started).count();
  const auto average_ms = total_ms / static_cast<double>(iterations);
  const auto megapixels = static_cast<double>(source.cols) * static_cast<double>(source.rows) *
                          static_cast<double>(iterations) / 1'000'000.0;
  const auto throughput = total_ms > 0.0 ? megapixels / (total_ms / 1000.0) : 0.0;

  return {
      {"name", method.name},
      {"label", method.label},
      {"totalMs", total_ms},
      {"averageMs", average_ms},
      {"megapixelsPerSecond", throughput},
      {"checksum", last_checksum},
  };
}

std::string fastest_method_name(const nlohmann::json& methods) {
  std::string fastest;
  double best_ms = std::numeric_limits<double>::max();
  for (const auto& method : methods) {
    const auto average_ms = method.value("averageMs", best_ms);
    if (average_ms < best_ms) {
      best_ms = average_ms;
      fastest = method.value("name", std::string{});
    }
  }
  return fastest;
}

double speedup_vs_pointer(const nlohmann::json& methods) {
  double pointer_ms = 0.0;
  double for_each_ms = 0.0;
  for (const auto& method : methods) {
    const auto name = method.value("name", std::string{});
    if (name == "pointerLoop") {
      pointer_ms = method.value("averageMs", 0.0);
    }
    if (name == "opencvForEach") {
      for_each_ms = method.value("averageMs", 0.0);
    }
  }
  if (pointer_ms <= 0.0 || for_each_ms <= 0.0) {
    return 0.0;
  }
  return pointer_ms / for_each_ms;
}

}  // namespace

PerformanceBenchmarkService::PerformanceBenchmarkService(
    ImageResultStore& image_store, PerformanceBenchmarkStore& benchmark_store)
    : image_store_(image_store),
      benchmark_store_(benchmark_store) {}

nlohmann::json PerformanceBenchmarkService::run_pixel_access_benchmark(
    const std::string& image_result_id,
    int iterations,
    int max_iterations,
    const std::function<bool()>& is_cancelled,
    const std::function<void(int, const std::string&)>& report_progress) {
  const auto image_metadata = image_store_.get(image_result_id);
  if (!image_metadata) {
    throw std::runtime_error("Image result was not found.");
  }

  const auto preview = image_store_.preview(image_result_id, "result");
  if (!preview || preview->empty()) {
    throw std::runtime_error("Image result preview was not available.");
  }

  cv::Mat source = to_bgr(*preview);
  if (source.empty()) {
    throw std::runtime_error("OpenCV could not normalize the image for benchmarking.");
  }
  if (source.depth() != CV_8U) {
    source.convertTo(source, CV_8U);
  }
  if (!source.isContinuous()) {
    source = source.clone();
  }

  const int repeat_count = std::clamp(iterations, 1, std::clamp(max_iterations, 1, 100));
  const std::vector<BenchmarkMethod> methods = {
      {"pointerLoop", "Manual pointer loop", invert_with_pointer_loop},
      {"opencvForEach", "OpenCV forEach pixel loop", invert_with_for_each},
      {"opencvBitwiseNot", "OpenCV bitwise_not baseline", invert_with_bitwise_not},
  };

  nlohmann::json measured_methods = nlohmann::json::array();
  for (std::size_t index = 0; index < methods.size(); ++index) {
    if (is_cancelled()) {
      throw std::runtime_error("Benchmark job was cancelled.");
    }
    const auto progress =
        10 + static_cast<int>((static_cast<double>(index) / static_cast<double>(methods.size())) * 80.0);
    report_progress(progress, "Running " + methods[index].label + ".");
    measured_methods.push_back(measure_method(methods[index], source, repeat_count));
  }

  report_progress(95, "Recording benchmark sample.");
  nlohmann::json benchmark = {
      {"operation", "pixelInvertBenchmark"},
      {"imageResultId", image_result_id},
      {"imageName", image_metadata->value("name", std::string{"image"})},
      {"width", source.cols},
      {"height", source.rows},
      {"channels", source.channels()},
      {"iterations", repeat_count},
      {"pixelCount", static_cast<long long>(source.cols) * static_cast<long long>(source.rows)},
      {"methods", measured_methods},
      {"fastestMethod", fastest_method_name(measured_methods)},
      {"forEachSpeedupVsPointer", speedup_vs_pointer(measured_methods)},
  };

  return benchmark_store_.record(benchmark);
}

}  // namespace app
