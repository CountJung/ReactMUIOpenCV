#pragma once

#include "../image/ImageResultStore.h"

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <string>

namespace app {

class ContourExtractionService {
public:
  explicit ContourExtractionService(ImageResultStore& image_store);

  nlohmann::json detect_candidates(const std::string& image_result_id, const nlohmann::json& params) const;
  cv::Mat preview_candidate(const std::string& image_result_id, const nlohmann::json& candidate) const;
  nlohmann::json extract_candidate(const std::string& image_result_id, const nlohmann::json& candidate) const;

private:
  ImageResultStore& image_store_;
};

}  // namespace app
