#include "ApiServer.h"

#include "../common/HttpUtils.h"

#include <opencv2/imgcodecs.hpp>

#include <vector>

namespace app {

void ApiServer::register_contour_routes(RequestGuard is_loopback_or_control) {
  server_.Get("/api/contours/ocr/languages", [this](const httplib::Request& request, httplib::Response& response) {
    try {
      auto data = contour_extraction_service_.available_ocr_languages();
      if (!is_loopback_request(request)) {
        data["tessdataPath"] = "";
      }
      send_data(response, data);
    } catch (const std::exception& error) {
      log_store_.append("error", "Tesseract language discovery failed: " + std::string(error.what()));
      send_error(response, "contour_ocr_language_discovery_failed", error.what(), 400);
    }
  });

  server_.Get("/api/contours/candidates", [this](const httplib::Request& request, httplib::Response& response) {
    const auto result_id = request.has_param("resultId") ? request.get_param_value("resultId") : std::string{};
    if (result_id.empty()) {
      send_error(response, "invalid_contour_candidate_request", "resultId is required.", 400);
      return;
    }

    nlohmann::json params = settings_store_.get()
                                .value("opencv", nlohmann::json::object())
                                .value("contourDetection", nlohmann::json::object());
    const std::vector<std::pair<std::string, std::string>> numeric_params = {
        {"maxCandidates", "maxCandidates"},
        {"low", "low"},
        {"high", "high"},
        {"minAreaRatio", "minAreaRatio"},
        {"epsilon", "epsilon"},
    };
    for (const auto& [query_name, param_name] : numeric_params) {
      if (!request.has_param(query_name)) {
        continue;
      }
      try {
        if (query_name == "maxCandidates") {
          params[param_name] = std::stoi(request.get_param_value(query_name));
        } else {
          params[param_name] = std::stod(request.get_param_value(query_name));
        }
      } catch (const std::exception&) {
        send_error(response, "invalid_contour_candidate_request", query_name + " must be numeric.", 400);
        return;
      }
    }
    if (request.has_param("retrieval")) {
      params["retrieval"] = request.get_param_value("retrieval");
    }

    try {
      send_data(response, contour_extraction_service_.detect_candidates(result_id, params));
    } catch (const std::exception& error) {
      log_store_.append("error", "Contour candidate detection failed: " + std::string(error.what()));
      send_error(response, "contour_candidate_detection_failed", error.what(), 400);
    }
  });

  server_.Post(
      "/api/contours/preview",
      [this, is_loopback_or_control](const httplib::Request& request, httplib::Response& response) {
        if (!is_loopback_or_control(request, response)) {
          return;
        }

        const auto body = parse_body(request);
        const auto result_id = body.value("resultId", std::string{});
        const auto candidate =
            body.contains("candidate") && body["candidate"].is_object() ? body["candidate"] : nlohmann::json::object();
        if (result_id.empty()) {
          send_error(response, "invalid_contour_preview_request", "resultId is required.", 400);
          return;
        }

        try {
          const auto preview = contour_extraction_service_.preview_candidate(result_id, candidate);
          std::vector<unsigned char> encoded;
          if (!cv::imencode(".png", preview, encoded)) {
            send_error(response, "contour_preview_failed", "OpenCV failed to encode the contour preview.", 500);
            return;
          }

          set_cors(response);
          response.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), "image/png");
        } catch (const std::exception& error) {
          log_store_.append("error", "Contour preview failed: " + std::string(error.what()));
          send_error(response, "contour_preview_failed", error.what(), 400);
        }
      });

  server_.Post(
      "/api/contours/ocr",
      [this, is_loopback_or_control](const httplib::Request& request, httplib::Response& response) {
        if (!is_loopback_or_control(request, response)) {
          return;
        }

        const auto body = parse_body(request);
        const auto result_id = body.value("resultId", std::string{});
        const auto candidate =
            body.contains("candidate") && body["candidate"].is_object() ? body["candidate"] : nlohmann::json::object();
        nlohmann::json ocr_params = nlohmann::json::object();
        if (body.contains("languages")) {
          ocr_params["languages"] = body["languages"];
        }
        if (body.contains("language")) {
          ocr_params["language"] = body["language"];
        }
        if (body.contains("pageSegMode")) {
          ocr_params["pageSegMode"] = body["pageSegMode"];
        }
        if (result_id.empty()) {
          send_error(response, "invalid_contour_ocr_request", "resultId is required.", 400);
          return;
        }

        try {
          const auto ocr = contour_extraction_service_.recognize_candidate_text(result_id, candidate, ocr_params);
          log_store_.append("info", "Recognized contour text from image " + result_id);
          send_data(response, ocr);
        } catch (const std::exception& error) {
          log_store_.append("error", "Contour OCR failed: " + std::string(error.what()));
          send_error(response, "contour_ocr_failed", error.what(), 400);
        }
      });

  server_.Post(
      "/api/contours/extract",
      [this, is_loopback_or_control](const httplib::Request& request, httplib::Response& response) {
        if (!is_loopback_or_control(request, response)) {
          return;
        }

        const auto body = parse_body(request);
        const auto result_id = body.value("resultId", std::string{});
        const auto candidate =
            body.contains("candidate") && body["candidate"].is_object() ? body["candidate"] : nlohmann::json::object();
        if (result_id.empty()) {
          send_error(response, "invalid_contour_extraction_request", "resultId is required.", 400);
          return;
        }

        const auto job =
            job_queue_.create_manual("contour-extraction", "Extracting perspective contour from " + result_id);
        const auto job_id = job.value("id", std::string{});
        job_queue_.start(job_id, "Perspective extraction started.");
        try {
          job_queue_.progress(job_id, 35, "Warping selected contour.");
          const auto result = contour_extraction_service_.extract_candidate(result_id, candidate);
          const auto completed_job = job_queue_.complete(job_id, "Perspective extraction completed.");
          event_hub_.publish("preview.image.updated", result);
          event_hub_.publish("contour.extraction.created", result);
          log_store_.append("info", "Extracted contour perspective from image " + result_id);
          send_data(response, {{"job", completed_job.value_or(job)}, {"result", result}}, 202);
        } catch (const std::exception& error) {
          job_queue_.fail(job_id, error.what());
          log_store_.append("error", "Contour perspective extraction failed: " + std::string(error.what()));
          send_error(response, "contour_extraction_failed", error.what(), 400);
        }
      });
}

void ApiServer::register_performance_routes(RequestGuard is_loopback_or_control) {
  server_.Get("/api/performance/benchmarks", [this](const httplib::Request& request, httplib::Response& response) {
    send_data(response, performance_benchmark_store_.list(is_loopback_request(request)));
  });

  server_.Post(
      "/api/performance/benchmarks/run",
      [this, is_loopback_or_control](const httplib::Request& request, httplib::Response& response) {
        if (!is_loopback_or_control(request, response)) {
          return;
        }

        const auto benchmark_settings =
            settings_store_.get()
                .value("opencv", nlohmann::json::object())
                .value("performanceBenchmark", nlohmann::json{{"defaultIterations", 5}, {"maxIterations", 20}});
        const auto body = parse_body(request);
        const auto result_id = body.value("resultId", std::string{});
        const auto iterations = body.value("iterations", benchmark_settings.value("defaultIterations", 5));
        const auto max_iterations = benchmark_settings.value("maxIterations", 20);
        if (result_id.empty()) {
          send_error(response, "invalid_performance_benchmark_request", "resultId is required.", 400);
          return;
        }

        const auto job =
            job_queue_.create_manual("performance-benchmark", "Benchmarking image result " + result_id + ".");
        const auto job_id = job.value("id", std::string{});
        job_queue_.start(job_id, "OpenCV pixel benchmark started.");
        try {
          const auto benchmark = performance_benchmark_service_.run_pixel_access_benchmark(
              result_id,
              iterations,
              max_iterations,
              [&]() { return job_queue_.is_cancelled(job_id); },
              [&](int progress, const std::string& message) { job_queue_.progress(job_id, progress, message); });
          const auto completed_job = job_queue_.complete(job_id, "OpenCV pixel benchmark completed.");
          event_hub_.publish("performance.benchmark.recorded", benchmark);
          log_store_.append("info", "Recorded OpenCV pixel benchmark for image " + result_id);
          send_data(response, {{"job", completed_job.value_or(job)}, {"benchmark", benchmark}}, 202);
        } catch (const std::exception& error) {
          if (!job_queue_.is_cancelled(job_id)) {
            job_queue_.fail(job_id, error.what());
          }
          log_store_.append("error", "OpenCV pixel benchmark failed: " + std::string(error.what()));
          send_error(
              response, "performance_benchmark_failed", error.what(), job_queue_.is_cancelled(job_id) ? 409 : 400);
        }
      });

  server_.Delete(
      R"(/api/performance/benchmarks/([A-Za-z0-9_-]+))",
      [this, is_loopback_or_control](const httplib::Request& request, httplib::Response& response) {
        if (!is_loopback_or_control(request, response)) {
          return;
        }

        const auto id = request.matches[1].str();
        if (!performance_benchmark_store_.remove(id)) {
          send_error(response, "performance_benchmark_not_found", "Benchmark record was not found.", 404);
          return;
        }
        log_store_.append("info", "Removed performance benchmark " + id);
        send_data(response, {{"deleted", id}});
      });

  server_.Delete(
      "/api/performance/benchmarks",
      [this, is_loopback_or_control](const httplib::Request& request, httplib::Response& response) {
        if (!is_loopback_or_control(request, response)) {
          return;
        }

        const auto deleted = performance_benchmark_store_.clear();
        log_store_.append("info", "Cleared performance benchmark history.");
        send_data(response, {{"deleted", deleted}});
      });
}

}  // namespace app
