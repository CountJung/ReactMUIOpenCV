#pragma once

#include "../image/ImageResultStore.h"
#include "../storage/PerformanceBenchmarkStore.h"

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

namespace app {

class PerformanceBenchmarkService {
public:
  PerformanceBenchmarkService(ImageResultStore& image_store, PerformanceBenchmarkStore& benchmark_store);

  nlohmann::json run_pixel_access_benchmark(
      const std::string& image_result_id,
      int iterations,
      const std::function<bool()>& is_cancelled,
      const std::function<void(int, const std::string&)>& report_progress);

private:
  ImageResultStore& image_store_;
  PerformanceBenchmarkStore& benchmark_store_;
};

}  // namespace app
