#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

namespace app {

nlohmann::json list_dnn_model_packages();
nlohmann::json download_dnn_model_package(
    const std::string& package_id,
    const std::function<bool()>& should_cancel,
    const std::function<void(int, const std::string&)>& report_progress);

}  // namespace app
