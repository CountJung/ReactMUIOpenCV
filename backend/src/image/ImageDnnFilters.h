#pragma once

#include "ImageFilters.h"

#include <nlohmann/json.hpp>

#include <string>

namespace app {

bool is_dnn_operation(const std::string& operation);
ImageOperationResult
apply_dnn_operation(const cv::Mat& source, const std::string& operation, const nlohmann::json& params);
nlohmann::json list_dnn_model_assets();

}  // namespace app
