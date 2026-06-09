#include "ApiServer.h"

#include "../common/HttpUtils.h"
#include "../common/Random.h"

#include <fstream>
#include <filesystem>
#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sstream>
#include <vector>

namespace app {

ApiServer::ApiServer(
    std::string host,
    int port,
    int ws_port,
    std::vector<std::string> lan_addresses,
    EventHub& event_hub,
    LogStore& log_store,
    JobQueue& job_queue,
    SettingsStore& settings_store,
    RemoteAccessManager& remote_access,
    ImageResultStore& image_store,
    VideoService& video_service,
    PipelineStore& pipeline_store,
    std::filesystem::path static_root)
    : host_(std::move(host)),
      port_(port),
      ws_port_(ws_port),
      lan_addresses_(std::move(lan_addresses)),
      event_hub_(event_hub),
      log_store_(log_store),
      job_queue_(job_queue),
      settings_store_(settings_store),
      remote_access_(remote_access),
      image_store_(image_store),
      video_service_(video_service),
      pipeline_store_(pipeline_store),
      static_root_(std::move(static_root)) {
  register_routes();
  mount_static_files();
}

bool ApiServer::listen() {
  log_store_.append("info", "HTTP backend listening on http://" + host_ + ":" + std::to_string(port_));
  return server_.listen(host_, port_);
}

void ApiServer::register_routes() {
  server_.Options(".*", [](const httplib::Request&, httplib::Response& response) {
    set_cors(response);
  });

  server_.Get("/api/health", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {
                            {"status", "ok"},
                            {"service", "ReactMUIOpenCV"},
                            {"opencvVersion", CV_VERSION},
                        });
  });

  server_.Get("/api/server-info", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {
                            {"service", "ReactMUIOpenCV"},
                            {"httpUrl", "http://" + host_ + ":" + std::to_string(port_)},
                            {"wsUrl", "ws://" + host_ + ":" + std::to_string(ws_port_)},
                            {"opencvVersion", CV_VERSION},
                        });
  });

  server_.Get("/api/network-info", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {
                            {"hostName", "localhost"},
                            {"bindHost", host_},
                            {"port", port_},
                            {"webSocketPort", ws_port_},
                            {"lanBound", remote_access_.lan_bound()},
                            {"selectedIp", remote_access_.selected_ip()},
                            {"addresses", lan_addresses_},
                        });
  });

  server_.Get("/api/settings", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, settings_store_.get());
  });

  server_.Put("/api/settings", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto next_mode = body.value("themeMode", std::string{"system"});
    const auto updated = settings_store_.update_theme_mode(next_mode);
    if (!updated) {
      send_error(response, "invalid_settings", "themeMode must be light, dark, or system.", 400);
      return;
    }

    log_store_.append("info", "Settings updated");
    send_data(response, *updated);
  });

  server_.Get("/api/remote/status", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, remote_access_.status(port_));
  });

  server_.Post("/api/remote/enable", [&](const httplib::Request&, httplib::Response& response) {
    auto body = remote_access_.enable(port_);
    body["message"] = remote_access_.lan_bound()
                          ? "LAN Web UI is enabled."
                          : "Remote access is enabled, but restart with --lan to bind externally.";
    event_hub_.publish("backend.status.changed", {{"remoteAccess", "enabled"}});
    log_store_.append("warning", "LAN Web UI mode enabled");
    send_data(response, body);
  });

  server_.Post("/api/remote/disable", [&](const httplib::Request&, httplib::Response& response) {
    auto body = remote_access_.disable(port_);
    body["message"] = "LAN Web UI is disabled and clients were disconnected.";
    event_hub_.publish("backend.status.changed", {{"remoteAccess", "disabled"}});
    log_store_.append("info", "LAN Web UI mode disabled");
    send_data(response, body);
  });

  server_.Post("/api/remote/rotate-token", [&](const httplib::Request&, httplib::Response& response) {
    auto body = remote_access_.rotate_token(port_);
    body["message"] = "Access token and PIN rotated. Existing remote clients were disconnected.";
    event_hub_.publish("remote.client.disconnected", {{"reason", "token_rotated"}});
    log_store_.append("warning", "Remote access token rotated");
    send_data(response, body);
  });

  server_.Get("/api/remote/clients", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, remote_access_.clients());
  });

  server_.Post("/api/remote/disconnect-all", [&](const httplib::Request&, httplib::Response& response) {
    const auto body = remote_access_.disconnect_all();
    event_hub_.publish("remote.client.disconnected", {{"reason", "disconnect_all"}});
    log_store_.append("warning", "All remote clients disconnected");
    send_data(response, body);
  });

  server_.Post("/api/remote/auth", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto token = body.value("token", std::string{});
    const auto pin = body.value("pin", std::string{});

    const auto auth_result = remote_access_.authenticate(request.remote_addr, token, pin);
    if (!auth_result) {
      log_store_.append("warning", "Remote client authentication failed from " + request.remote_addr);
      send_error(response, "invalid_remote_credentials", "Remote token or PIN is invalid.", 401);
      return;
    }

    event_hub_.publish("remote.client.connected", (*auth_result)["client"]);
    log_store_.append("info", "Remote read-only client connected from " + request.remote_addr);
    send_data(response, {
                            {"sessionToken", (*auth_result)["sessionToken"]},
                            {"permission", (*auth_result)["permission"]},
                            {"expiresAt", (*auth_result)["expiresAt"]},
                        });
  });

  server_.Post("/api/files/open-local", [&](const httplib::Request&, httplib::Response& response) {
    send_error(response, "desktop_host_required", "Local file picker requires the Desktop Host.", 501);
  });

  server_.Post("/api/files/upload", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"fileId", random_hex(12)}, {"status", "accepted"}, {"storage", "temp"}}, 202);
  });

  server_.Get("/api/files/library", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"files", nlohmann::json::array()}});
  });

  server_.Delete(R"(/api/files/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, {{"deleted", request.matches[1].str()}});
  });

  server_.Post("/api/images/open-local", [&](const httplib::Request& request, httplib::Response& response) {
    if (!is_loopback_request(request)) {
      log_store_.append("warning", "Blocked remote local image open attempt from " + request.remote_addr);
      send_error(response, "desktop_only", "Local image paths are only available from the desktop host.", 403);
      return;
    }

    const auto body = parse_body(request);
    const auto path = body.value("path", std::string{});
    if (path.empty()) {
      send_error(response, "invalid_image_path", "A local image path is required.", 400);
      return;
    }

    try {
      const auto result = image_store_.open_local(std::filesystem::path(path));
      event_hub_.publish("preview.image.updated", result);
      log_store_.append("info", "Opened local image " + path);
      send_data(response, result, 201);
    } catch (const std::exception& error) {
      log_store_.append("error", "Local image open failed: " + std::string(error.what()));
      send_error(response, "image_open_failed", error.what(), 400);
    }
  });

  server_.Post("/api/images/upload", [&](const httplib::Request& request, httplib::Response& response) {
    try {
      if (!request.is_multipart_form_data() || !request.form.has_file("file")) {
        send_error(response, "invalid_upload", "Upload a multipart image file named 'file'.", 400);
        return;
      }

      const auto file = request.form.get_file("file");
      const auto result = image_store_.upload(file.filename, file.content);
      event_hub_.publish("preview.image.updated", result);
      log_store_.append("info", "Uploaded image " + file.filename);
      send_data(response, result, 201);
    } catch (const std::exception& error) {
      log_store_.append("error", "Image upload failed: " + std::string(error.what()));
      send_error(response, "image_upload_failed", error.what(), 400);
    }
  });

  server_.Get("/api/images/results", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, image_store_.list());
  });

  server_.Post("/api/images/process", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto result_id = body.value("resultId", std::string{});
    const auto operation = body.value("operation", std::string{});
    const auto params = body.contains("params") && body["params"].is_object() ? body["params"] : nlohmann::json::object();

    if (result_id.empty() || operation.empty()) {
      send_error(response, "invalid_image_process_request", "resultId and operation are required.", 400);
      return;
    }

    try {
      const auto result = image_store_.process(result_id, operation, params);
      const auto job = job_queue_.enqueue("image", "Applied " + operation + " to image result " + result_id + ".");
      event_hub_.publish("preview.image.updated", result);
      log_store_.append("info", "Applied image operation " + operation + " to " + result_id);
      send_data(response, {{"job", job}, {"result", result}}, 202);
    } catch (const std::exception& error) {
      log_store_.append("error", "Image processing failed: " + std::string(error.what()));
      send_error(response, "image_processing_failed", error.what(), 400);
    }
  });

  server_.Post("/api/images/save", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto result_id = body.value("resultId", std::string{});
    const auto format = body.value("format", std::string{"png"});

    if (result_id.empty()) {
      send_error(response, "invalid_image_save_request", "resultId is required.", 400);
      return;
    }

    try {
      const auto result = image_store_.save(result_id, format);
      log_store_.append("info", "Saved image result " + result_id);
      send_data(response, result);
    } catch (const std::exception& error) {
      log_store_.append("error", "Image save failed: " + std::string(error.what()));
      send_error(response, "image_save_failed", error.what(), 400);
    }
  });

  server_.Get(R"(/api/images/results/([A-Za-z0-9_-]+)/preview)", [&](const httplib::Request& request, httplib::Response& response) {
    const auto id = request.matches[1].str();
    const auto variant = request.has_param("variant") ? request.get_param_value("variant") : "result";
    const auto image = image_store_.preview(id, variant);
    if (!image) {
      send_error(response, "image_result_not_found", "Image result was not found.", 404);
      return;
    }

    std::vector<unsigned char> encoded;
    if (!cv::imencode(".png", *image, encoded)) {
      send_error(response, "image_preview_failed", "OpenCV failed to encode the preview.", 500);
      return;
    }

    set_cors(response);
    response.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), "image/png");
  });

  server_.Get(R"(/api/images/results/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto result = image_store_.get(request.matches[1].str());
    if (!result) {
      send_error(response, "image_result_not_found", "Image result was not found.", 404);
      return;
    }

    send_data(response, *result);
  });

  server_.Post("/api/videos/open-local", [&](const httplib::Request& request, httplib::Response& response) {
    if (!is_loopback_request(request)) {
      log_store_.append("warning", "Blocked remote local video open attempt from " + request.remote_addr);
      send_error(response, "desktop_only", "Local video paths are only available from the desktop host.", 403);
      return;
    }

    const auto body = parse_body(request);
    const auto path = body.value("path", std::string{});
    if (path.empty()) {
      send_error(response, "invalid_video_path", "A local video path is required.", 400);
      return;
    }

    try {
      const auto result = video_service_.open_local(std::filesystem::path(path));
      event_hub_.publish("preview.frame.updated", result);
      log_store_.append("info", "Opened local video " + path);
      send_data(response, result, 201);
    } catch (const std::exception& error) {
      log_store_.append("error", "Local video open failed: " + std::string(error.what()));
      send_error(response, "video_open_failed", error.what(), 400);
    }
  });

  server_.Post("/api/videos/upload", [&](const httplib::Request& request, httplib::Response& response) {
    try {
      if (!request.is_multipart_form_data() || !request.form.has_file("file")) {
        send_error(response, "invalid_upload", "Upload a multipart video file named 'file'.", 400);
        return;
      }

      const auto file = request.form.get_file("file");
      const auto result = video_service_.upload(file.filename, file.content);
      event_hub_.publish("preview.frame.updated", result);
      log_store_.append("info", "Uploaded video " + file.filename);
      send_data(response, result, 201);
    } catch (const std::exception& error) {
      log_store_.append("error", "Video upload failed: " + std::string(error.what()));
      send_error(response, "video_upload_failed", error.what(), 400);
    }
  });

  server_.Get("/api/videos", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, video_service_.list());
  });

  server_.Post("/api/videos/process", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto video_id = body.value("videoId", std::string{});
    const auto filter = body.value("filter", std::string{"none"});
    if (video_id.empty()) {
      send_error(response, "invalid_video_process_request", "videoId is required.", 400);
      return;
    }

    try {
      const auto video = video_service_.get(video_id);
      if (!video) {
        send_error(response, "video_not_found", "Video was not found.", 404);
        return;
      }

      const auto job = job_queue_.enqueue("video", "Queued " + filter + " frame filter preview for video " + video_id + ".");
      event_hub_.publish("preview.frame.updated", {{"videoId", video_id}, {"filter", filter}});
      log_store_.append("info", "Queued video process " + filter + " for " + video_id);
      send_data(response, {{"job", job}, {"video", *video}, {"filter", filter}}, 202);
    } catch (const std::exception& error) {
      log_store_.append("error", "Video processing failed: " + std::string(error.what()));
      send_error(response, "video_processing_failed", error.what(), 400);
    }
  });

  server_.Post("/api/videos/export", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto video_id = body.value("videoId", std::string{});
    const auto filter = body.value("filter", std::string{"none"});
    const auto start_frame = body.value("startFrame", 0);
    const auto end_frame = body.value("endFrame", 0);
    if (video_id.empty()) {
      send_error(response, "invalid_video_export_request", "videoId is required.", 400);
      return;
    }

    const auto job = job_queue_.enqueue("video-export", "Exporting " + filter + " video clip for " + video_id + ".");
    try {
      const auto result = video_service_.export_filtered_video(
          video_id,
          filter,
          start_frame,
          end_frame,
          [&](int progress) {
            event_hub_.publish("job.progress", {{"id", job["id"]}, {"type", "video-export"}, {"progress", progress}});
          });
      event_hub_.publish("job.completed", {{"id", job["id"]}, {"type", "video-export"}, {"progress", 100}});
      log_store_.append("info", "Exported video " + video_id + " with " + filter);
      send_data(response, {{"job", job}, {"result", result}}, 202);
    } catch (const std::exception& error) {
      event_hub_.publish("job.failed", {{"id", job["id"]}, {"type", "video-export"}, {"message", error.what()}});
      log_store_.append("error", "Video export failed: " + std::string(error.what()));
      send_error(response, "video_export_failed", error.what(), 400);
    }
  });

  server_.Post("/api/videos/extract-frame", [&](const httplib::Request& request, httplib::Response& response) {
    const auto body = parse_body(request);
    const auto video_id = body.value("videoId", std::string{});
    const auto frame_index = body.value("frameIndex", 0);
    const auto filter = body.value("filter", std::string{"none"});
    if (video_id.empty()) {
      send_error(response, "invalid_frame_extract_request", "videoId is required.", 400);
      return;
    }

    try {
      const auto result = video_service_.extract_frame(video_id, frame_index, filter);
      const auto job = job_queue_.enqueue("video-frame", "Extracted frame " + std::to_string(frame_index) + " from " + video_id + ".");
      event_hub_.publish("preview.frame.updated", result);
      log_store_.append("info", "Extracted video frame " + std::to_string(frame_index) + " from " + video_id);
      send_data(response, {{"job", job}, {"result", result}}, 202);
    } catch (const std::exception& error) {
      log_store_.append("error", "Video frame extraction failed: " + std::string(error.what()));
      send_error(response, "video_frame_extract_failed", error.what(), 400);
    }
  });

  server_.Get(R"(/api/videos/frame/([A-Za-z0-9_-]+)/([0-9]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto video_id = request.matches[1].str();
    const auto frame_index = std::stoi(request.matches[2].str());
    const auto filter = request.has_param("filter") ? request.get_param_value("filter") : "none";

    try {
      const auto frame = video_service_.read_frame(video_id, frame_index, filter);
      if (!frame) {
        send_error(response, "video_not_found", "Video was not found.", 404);
        return;
      }

      std::vector<unsigned char> encoded;
      if (!cv::imencode(".png", *frame, encoded)) {
        send_error(response, "video_frame_encode_failed", "OpenCV failed to encode the frame.", 500);
        return;
      }

      set_cors(response);
      response.set_content(reinterpret_cast<const char*>(encoded.data()), encoded.size(), "image/png");
    } catch (const std::exception& error) {
      log_store_.append("error", "Video frame preview failed: " + std::string(error.what()));
      send_error(response, "video_frame_preview_failed", error.what(), 400);
    }
  });

  server_.Get(R"(/api/videos/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto video = video_service_.get(request.matches[1].str());
    if (!video) {
      send_error(response, "video_not_found", "Video was not found.", 404);
      return;
    }

    send_data(response, *video);
  });

  server_.Post("/api/pipelines/execute", [&](const httplib::Request&, httplib::Response& response) {
    const auto job = job_queue_.enqueue("pipeline", "Pipeline execution placeholder job.");
    event_hub_.publish("pipeline.node.started", {{"jobId", job["id"]}, {"nodeId", "placeholder"}});
    send_data(response, {{"job", job}}, 202);
  });

  server_.Get("/api/pipelines", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, pipeline_store_.list());
  });

  server_.Post("/api/pipelines", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, pipeline_store_.create(parse_body(request)), 201);
  });

  server_.Put(R"(/api/pipelines/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, pipeline_store_.replace(request.matches[1].str(), parse_body(request)));
  });

  server_.Delete(R"(/api/pipelines/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    send_data(response, pipeline_store_.remove(request.matches[1].str()));
  });

  server_.Get("/api/jobs", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, job_queue_.list());
  });

  server_.Get(R"(/api/jobs/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    const auto job = job_queue_.get(request.matches[1].str());
    if (!job) {
      send_error(response, "job_not_found", "Job was not found.", 404);
      return;
    }
    send_data(response, *job);
  });

  server_.Post(R"(/api/jobs/([A-Za-z0-9_-]+)/cancel)", [&](const httplib::Request& request, httplib::Response& response) {
    if (!job_queue_.cancel(request.matches[1].str())) {
      send_error(response, "job_not_cancellable", "Job was not found or cannot be cancelled.", 404);
      return;
    }
    send_data(response, {{"cancelled", request.matches[1].str()}});
  });

  server_.Delete(R"(/api/jobs/([A-Za-z0-9_-]+))", [&](const httplib::Request& request, httplib::Response& response) {
    if (!job_queue_.remove(request.matches[1].str())) {
      send_error(response, "job_not_found", "Job was not found.", 404);
      return;
    }
    send_data(response, {{"deleted", request.matches[1].str()}});
  });

  server_.Get("/api/logs/recent", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, log_store_.recent());
  });

  server_.Get("/api/logs/download", [&](const httplib::Request&, httplib::Response& response) {
    set_cors(response);
    response.set_header("Content-Type", "text/plain");
    response.set_header("Content-Disposition", "attachment; filename=backend.log");
    std::ifstream log_file("logs/backend.log", std::ios::binary);
    std::ostringstream buffer;
    buffer << log_file.rdbuf();
    response.set_content(buffer.str(), "text/plain");
  });

  server_.Get("/api/events/recent", [&](const httplib::Request&, httplib::Response& response) {
    send_data(response, {{"events", event_hub_.recent()}});
  });
}

void ApiServer::mount_static_files() {
  if (std::filesystem::exists(static_root_)) {
    server_.set_mount_point("/", static_root_.string());
  } else {
    server_.Get("/", [](const httplib::Request&, httplib::Response& response) {
      response.set_content(
          "ReactMUIOpenCV backend is running. Build frontend/dist to serve the React UI.",
          "text/plain");
    });
  }
}

}  // namespace app
