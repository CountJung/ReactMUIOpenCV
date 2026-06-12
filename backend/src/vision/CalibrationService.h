#pragma once

#include "../image/ImageResultStore.h"
#include "../storage/CalibrationStore.h"

#include <nlohmann/json.hpp>
#include <string>

namespace app {

class CalibrationService {
public:
  CalibrationService(ImageResultStore& image_store, CalibrationStore& calibration_store);

  nlohmann::json calibrate_from_image(
      const std::string& result_id, int board_width, int board_height, double square_size);

private:
  ImageResultStore& image_store_;
  CalibrationStore& calibration_store_;
};

}  // namespace app
