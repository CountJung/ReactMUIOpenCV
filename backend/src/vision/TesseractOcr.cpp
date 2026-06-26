#include "TesseractOcr.h"

#include "../common/OpenCvUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <opencv2/imgproc.hpp>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>
#include <vector>

namespace app {

namespace {

struct TessdataInfo {
  std::filesystem::path path;
  std::vector<std::string> languages;
};

std::string language_label(const std::string& code) {
  static const std::map<std::string, std::string> labels = {
      {"eng", "English"},
      {"kor", "Korean"},
      {"jpn", "Japanese"},
      {"chi_sim", "Chinese Simplified"},
      {"chi_tra", "Chinese Traditional"},
      {"deu", "German"},
      {"fra", "French"},
      {"spa", "Spanish"},
      {"osd", "Orientation and Script Detection"},
  };
  const auto found = labels.find(code);
  return found == labels.end() ? code : found->second;
}

bool has_traineddata_file(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    return false;
  }

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file() && entry.path().extension() == ".traineddata") {
      return true;
    }
  }
  return false;
}

std::optional<std::string> environment_value(const char* name) {
#ifdef _WIN32
  char* raw = nullptr;
  std::size_t size = 0;
  if (_dupenv_s(&raw, &size, name) != 0 || !raw) {
    return std::nullopt;
  }
  std::unique_ptr<char, decltype(&std::free)> owned(raw, std::free);
  if (size <= 1) {
    return std::nullopt;
  }
  return std::string(owned.get());
#else
  const char* raw = std::getenv(name);
  if (!raw || !*raw) {
    return std::nullopt;
  }
  return std::string(raw);
#endif
}

std::optional<std::filesystem::path> traineddata_root(const std::filesystem::path& candidate) {
  if (candidate.empty()) {
    return std::nullopt;
  }

  std::error_code error;
  const auto normalized = std::filesystem::weakly_canonical(candidate, error);
  const auto root = error ? candidate.lexically_normal() : normalized;
  if (has_traineddata_file(root)) {
    return root;
  }

  const auto nested = root / "tessdata";
  if (has_traineddata_file(nested)) {
    return nested;
  }

  return std::nullopt;
}

std::vector<std::filesystem::path> tessdata_candidates() {
  std::vector<std::filesystem::path> candidates;
  if (const auto env = environment_value("TESSDATA_PREFIX")) {
    candidates.emplace_back(*env);
  }

#ifdef APP_TESSDATA_ROOT
  candidates.emplace_back(APP_TESSDATA_ROOT);
#endif

  const auto cwd = std::filesystem::current_path();
  candidates.push_back(cwd / "models" / "tessdata");
  candidates.push_back(cwd / ".tools" / "vcpkg" / "installed" / "x64-windows" / "share" / "tessdata");
  candidates.push_back(
      cwd / "backend" / "out" / "build" / "windows-msvc-vcpkg" / "vcpkg_installed" / "x64-windows" / "share" /
      "tessdata");
  candidates.push_back(cwd / "backend" / "vcpkg_installed" / "x64-windows" / "share" / "tessdata");
  return candidates;
}

TessdataInfo discover_tessdata() {
  for (const auto& candidate : tessdata_candidates()) {
    const auto root = traineddata_root(candidate);
    if (!root) {
      continue;
    }

    std::vector<std::string> languages;
    for (const auto& entry : std::filesystem::directory_iterator(*root)) {
      if (entry.is_regular_file() && entry.path().extension() == ".traineddata") {
        languages.push_back(entry.path().stem().string());
      }
    }
    std::sort(languages.begin(), languages.end());
    return {*root, languages};
  }

  return {};
}

bool is_language_code_safe(const std::string& value) {
  return !value.empty() && value.size() <= 32 &&
         std::all_of(value.begin(), value.end(), [](unsigned char ch) {
           return std::isalnum(ch) || ch == '_';
         });
}

std::vector<std::string> split_languages(const std::string& raw) {
  std::vector<std::string> result;
  std::stringstream stream(raw);
  std::string item;
  while (std::getline(stream, item, '+')) {
    item.erase(std::remove_if(item.begin(), item.end(), [](unsigned char ch) {
                 return std::isspace(ch) != 0;
               }),
               item.end());
    if (!item.empty()) {
      result.push_back(item);
    }
  }
  return result;
}

std::vector<std::string> default_languages(const std::vector<std::string>& installed) {
  const std::set<std::string> languages(installed.begin(), installed.end());
  if (languages.contains("eng") && languages.contains("kor")) {
    return {"eng", "kor"};
  }
  if (languages.contains("eng")) {
    return {"eng"};
  }
  if (!installed.empty()) {
    return {installed.front()};
  }
  return {"eng", "kor"};
}

std::vector<std::string> languages_from_params(const nlohmann::json& params, const std::vector<std::string>& installed) {
  std::vector<std::string> languages;
  if (params.contains("languages") && params["languages"].is_array()) {
    for (const auto& value : params["languages"]) {
      if (value.is_string()) {
        languages.push_back(value.get<std::string>());
      }
    }
  } else if (params.contains("languages") && params["languages"].is_string()) {
    languages = split_languages(params.value("languages", std::string{}));
  } else if (params.contains("language") && params["language"].is_string()) {
    languages = split_languages(params.value("language", std::string{}));
  } else {
    languages = default_languages(installed);
  }

  std::vector<std::string> sanitized;
  std::set<std::string> seen;
  const std::set<std::string> installed_set(installed.begin(), installed.end());
  for (const auto& language : languages) {
    if (!is_language_code_safe(language)) {
      throw std::runtime_error("Tesseract language codes may contain only letters, numbers, and underscores.");
    }
    if (!installed_set.empty() && !installed_set.contains(language)) {
      throw std::runtime_error("Tesseract language data is not installed: " + language);
    }
    if (seen.insert(language).second) {
      sanitized.push_back(language);
    }
  }

  if (sanitized.empty()) {
    throw std::runtime_error("At least one Tesseract language must be selected.");
  }
  return sanitized;
}

std::string join_languages(const std::vector<std::string>& languages) {
  std::string result;
  for (const auto& language : languages) {
    if (!result.empty()) {
      result += '+';
    }
    result += language;
  }
  return result;
}

tesseract::PageSegMode page_seg_mode_from_params(const nlohmann::json& params) {
  const auto value = params.value("pageSegMode", std::string{"auto"});
  if (value == "single-line") {
    return tesseract::PSM_SINGLE_LINE;
  }
  if (value == "single-word") {
    return tesseract::PSM_SINGLE_WORD;
  }
  if (value == "single-block") {
    return tesseract::PSM_SINGLE_BLOCK;
  }
  if (value == "sparse") {
    return tesseract::PSM_SPARSE_TEXT;
  }
  return tesseract::PSM_AUTO;
}

std::string page_seg_mode_name(tesseract::PageSegMode mode) {
  switch (mode) {
    case tesseract::PSM_SINGLE_LINE:
      return "single-line";
    case tesseract::PSM_SINGLE_WORD:
      return "single-word";
    case tesseract::PSM_SINGLE_BLOCK:
      return "single-block";
    case tesseract::PSM_SPARSE_TEXT:
      return "sparse";
    default:
      return "auto";
  }
}

cv::Mat prepare_tesseract_image(const cv::Mat& source) {
  cv::Mat gray = to_gray(source);
  const double max_side = static_cast<double>(std::max(gray.cols, gray.rows));
  if (max_side > 0.0 && max_side < 1400.0) {
    const double scale = std::clamp(1400.0 / max_side, 1.0, 3.0);
    cv::resize(gray, gray, cv::Size(), scale, scale, cv::INTER_CUBIC);
  }

  cv::Mat equalized;
  cv::createCLAHE(2.0, cv::Size(8, 8))->apply(gray, equalized);
  cv::GaussianBlur(equalized, equalized, cv::Size(3, 3), 0.0);
  return equalized.isContinuous() ? equalized : equalized.clone();
}

std::string tesseract_text(char* raw) {
  if (!raw) {
    return {};
  }
  std::unique_ptr<char[]> owned(raw);
  return std::string(owned.get());
}

nlohmann::json collect_tesseract_lines(tesseract::TessBaseAPI& api) {
  auto lines = nlohmann::json::array();
  auto* iterator = api.GetIterator();
  if (!iterator) {
    return lines;
  }

  do {
    if (iterator->Empty(tesseract::RIL_TEXTLINE)) {
      continue;
    }
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    iterator->BoundingBox(tesseract::RIL_TEXTLINE, &x1, &y1, &x2, &y2);
    const auto text = tesseract_text(iterator->GetUTF8Text(tesseract::RIL_TEXTLINE));
    if (text.empty()) {
      continue;
    }
    lines.push_back(
        {{"text", text},
         {"confidence", static_cast<double>(iterator->Confidence(tesseract::RIL_TEXTLINE)) / 100.0},
         {"boundingBox", {{"x", x1}, {"y", y1}, {"width", std::max(0, x2 - x1)}, {"height", std::max(0, y2 - y1)}}}});
  } while (iterator->Next(tesseract::RIL_TEXTLINE));

  return lines;
}

int count_tesseract_words(tesseract::TessBaseAPI& api) {
  int count = 0;
  std::unique_ptr<int[]> confidences(api.AllWordConfidences());
  if (!confidences) {
    return count;
  }
  while (confidences[count] != -1) {
    ++count;
  }
  return count;
}

}  // namespace

nlohmann::json list_tesseract_ocr_languages() {
  const auto tessdata = discover_tessdata();
  auto languages = nlohmann::json::array();
  for (const auto& code : tessdata.languages) {
    languages.push_back({{"code", code}, {"label", language_label(code)}});
  }

  const auto defaults = default_languages(tessdata.languages);
  return {
      {"tessdataPath", tessdata.path.empty() ? "" : tessdata.path.lexically_normal().string()},
      {"tessdataAvailable", !tessdata.path.empty()},
      {"availableLanguages", languages},
      {"defaultLanguages", defaults},
      {"recommendedLanguages", nlohmann::json::array({nlohmann::json::array({"eng", "kor"}), nlohmann::json::array({"eng"})})},
  };
}

nlohmann::json recognize_tesseract_ocr_text(const cv::Mat& source, const nlohmann::json& params) {
  if (source.empty()) {
    throw std::runtime_error("Selected contour preview was empty.");
  }

  const auto tessdata = discover_tessdata();
  if (tessdata.path.empty()) {
    throw std::runtime_error(
        "Tesseract tessdata was not found. Add traineddata files under models/tessdata or set TESSDATA_PREFIX.");
  }

  const auto languages = languages_from_params(params, tessdata.languages);
  const auto language_string = join_languages(languages);
  const auto page_seg_mode = page_seg_mode_from_params(params);

  tesseract::TessBaseAPI api;
  if (api.Init(tessdata.path.string().c_str(), language_string.c_str(), tesseract::OEM_LSTM_ONLY) != 0) {
    throw std::runtime_error("Tesseract failed to initialize languages: " + language_string);
  }
  api.SetPageSegMode(page_seg_mode);

  const auto prepared = prepare_tesseract_image(source);
  api.SetImage(prepared.data, prepared.cols, prepared.rows, 1, static_cast<int>(prepared.step));
  api.SetSourceResolution(300);
  if (api.Recognize(nullptr) != 0) {
    api.End();
    throw std::runtime_error("Tesseract recognition failed.");
  }

  const auto text = tesseract_text(api.GetUTF8Text());
  const int mean_confidence = api.MeanTextConf();
  const auto lines = collect_tesseract_lines(api);
  const auto word_count = count_tesseract_words(api);
  api.End();

  return {
      {"text", text},
      {"confidence", std::clamp(mean_confidence, 0, 100) / 100.0},
      {"lineCount", lines.size()},
      {"wordCount", word_count},
      {"imageSize", {{"width", prepared.cols}, {"height", prepared.rows}}},
      {"method",
       {{"engine", "tesseract"},
        {"languages", languages},
        {"language", language_string},
        {"pageSegMode", page_seg_mode_name(page_seg_mode)},
        {"tessdataPath", tessdata.path.lexically_normal().string()},
        {"preprocessing", "grayscale-clahe-upscale"}}},
      {"lines", lines},
  };
}

}  // namespace app
