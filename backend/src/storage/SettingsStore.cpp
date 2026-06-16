#include "SettingsStore.h"

#include <fstream>
#include <utility>

namespace app {

nlohmann::json SettingsStore::defaults() {
  return {
      {"themeMode", "system"},
      {"opencv",
       {{"contourDetection",
         {
             {"maxCandidates", 24},
             {"low", 45.0},
             {"high", 150.0},
             {"minAreaRatio", 0.002},
             {"epsilon", 0.024},
             {"retrieval", "list"},
         }},
        {"performanceBenchmark", {{"defaultIterations", 5}, {"maxIterations", 20}}}}},
      {"logging", {{"level", "info"}, {"maxFileMb", 10}, {"retentionDays", 5}}},
  };
}

namespace {

bool is_theme_mode_valid(const std::string& theme_mode) {
  return theme_mode == "light" || theme_mode == "dark" || theme_mode == "system";
}

bool is_retrieval_valid(const std::string& retrieval) {
  return retrieval == "list" || retrieval == "external";
}

bool is_log_level_valid(const std::string& level) {
  return level == "debug" || level == "info" || level == "warning" || level == "error";
}

std::optional<nlohmann::json> normalize_contour_detection(const nlohmann::json& input) {
  if (!input.is_object()) {
    return std::nullopt;
  }

  const auto max_candidates = input.value("maxCandidates", 0);
  const auto low = input.value("low", 0.0);
  const auto high = input.value("high", 0.0);
  const auto min_area_ratio = input.value("minAreaRatio", 0.0);
  const auto epsilon = input.value("epsilon", 0.0);
  const auto retrieval = input.value("retrieval", std::string{});

  if (max_candidates < 1 || max_candidates > 80 || low < 1.0 || low > 255.0 || high <= low || high > 255.0 ||
      min_area_ratio < 0.0001 || min_area_ratio > 0.5 || epsilon < 0.005 || epsilon > 0.12 ||
      !is_retrieval_valid(retrieval)) {
    return std::nullopt;
  }

  return nlohmann::json{
      {"maxCandidates", max_candidates},
      {"low", low},
      {"high", high},
      {"minAreaRatio", min_area_ratio},
      {"epsilon", epsilon},
      {"retrieval", retrieval},
  };
}

std::optional<nlohmann::json> normalize_logging(const nlohmann::json& input) {
  if (!input.is_object()) {
    return std::nullopt;
  }

  const auto level = input.value("level", std::string{"info"});
  const auto max_file_mb = input.value("maxFileMb", 0);
  const auto retention_days = input.value("retentionDays", 0);
  if (!is_log_level_valid(level) || max_file_mb < 1 || max_file_mb > 100 || retention_days < 1 || retention_days > 30) {
    return std::nullopt;
  }

  return nlohmann::json{{"level", level}, {"maxFileMb", max_file_mb}, {"retentionDays", retention_days}};
}

std::optional<nlohmann::json> normalize_performance_benchmark(const nlohmann::json& input) {
  if (!input.is_object()) {
    return std::nullopt;
  }

  const auto default_iterations = input.value("defaultIterations", 0);
  const auto max_iterations = input.value("maxIterations", 0);
  if (default_iterations < 1 || default_iterations > 100 || max_iterations < default_iterations ||
      max_iterations > 100) {
    return std::nullopt;
  }

  return nlohmann::json{{"defaultIterations", default_iterations}, {"maxIterations", max_iterations}};
}

void merge_known_settings(nlohmann::json& target, const nlohmann::json& patch) {
  if (patch.contains("themeMode")) {
    target["themeMode"] = patch["themeMode"];
  }

  if (patch.contains("opencv") && patch["opencv"].is_object() && patch["opencv"].contains("contourDetection")) {
    target["opencv"]["contourDetection"] = patch["opencv"]["contourDetection"];
  }
  if (patch.contains("opencv") && patch["opencv"].is_object() && patch["opencv"].contains("performanceBenchmark")) {
    target["opencv"]["performanceBenchmark"] = patch["opencv"]["performanceBenchmark"];
  }

  if (patch.contains("logging")) {
    target["logging"] = patch["logging"];
  }
}

}  // namespace

SettingsStore::SettingsStore(std::filesystem::path storage_path)
    : storage_path_(std::move(storage_path)),
      settings_(defaults()) {
  load();
}

nlohmann::json SettingsStore::get() const {
  std::scoped_lock lock(mutex_);
  return settings_;
}

std::optional<nlohmann::json> SettingsStore::update(const nlohmann::json& patch) {
  std::scoped_lock lock(mutex_);
  try {
    auto next = settings_;
    merge_known_settings(next, patch);

    const auto theme_mode = next.value("themeMode", std::string{"system"});
    if (!is_theme_mode_valid(theme_mode)) {
      return std::nullopt;
    }

    const auto contour = normalize_contour_detection(next["opencv"]["contourDetection"]);
    const auto performance_benchmark = normalize_performance_benchmark(next["opencv"]["performanceBenchmark"]);
    const auto logging = normalize_logging(next["logging"]);
    if (!contour || !performance_benchmark || !logging) {
      return std::nullopt;
    }
    next["themeMode"] = theme_mode;
    next["opencv"]["contourDetection"] = *contour;
    next["opencv"]["performanceBenchmark"] = *performance_benchmark;
    next["logging"] = *logging;

    settings_ = next;
    save_locked();
    return settings_;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

void SettingsStore::load() {
  std::scoped_lock lock(mutex_);
  if (!std::filesystem::exists(storage_path_)) {
    save_locked();
    return;
  }

  std::ifstream input(storage_path_);
  if (!input) {
    return;
  }

  auto loaded = nlohmann::json::parse(input, nullptr, false);
  if (loaded.is_discarded() || !loaded.is_object()) {
    return;
  }

  auto next = defaults();
  try {
    merge_known_settings(next, loaded);
  } catch (const std::exception&) {
    return;
  }
  const auto contour = normalize_contour_detection(next["opencv"]["contourDetection"]);
  const auto performance_benchmark = normalize_performance_benchmark(next["opencv"]["performanceBenchmark"]);
  const auto logging = normalize_logging(next["logging"]);
  if (!is_theme_mode_valid(next.value("themeMode", std::string{"system"})) || !contour || !performance_benchmark ||
      !logging) {
    return;
  }

  next["opencv"]["contourDetection"] = *contour;
  next["opencv"]["performanceBenchmark"] = *performance_benchmark;
  next["logging"] = *logging;
  settings_ = next;
}

void SettingsStore::save_locked() const {
  std::filesystem::create_directories(storage_path_.parent_path());
  std::ofstream output(storage_path_);
  if (output) {
    output << settings_.dump(2);
  }
}

}  // namespace app
