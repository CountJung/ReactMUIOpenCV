#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

namespace app {

nlohmann::json list_tesseract_ocr_languages();
nlohmann::json recognize_tesseract_ocr_text(const cv::Mat& source, const nlohmann::json& params);

}  // namespace app
