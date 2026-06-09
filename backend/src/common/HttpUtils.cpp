#include "HttpUtils.h"

namespace app {

nlohmann::json api_ok(const nlohmann::json& data) {
  return {{"ok", true}, {"data", data}, {"error", nullptr}};
}

nlohmann::json api_error(const std::string& code, const std::string& message) {
  return {{"ok", false}, {"data", nullptr}, {"error", {{"code", code}, {"message", message}}}};
}

void set_cors(httplib::Response& response) {
  response.set_header("Access-Control-Allow-Origin", "*");
  response.set_header("Access-Control-Allow-Headers", "Content-Type, X-Remote-Session");
  response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
}

void send_json(httplib::Response& response, const nlohmann::json& body, int status) {
  response.status = status;
  set_cors(response);
  response.set_content(body.dump(), "application/json");
}

void send_data(httplib::Response& response, const nlohmann::json& data, int status) {
  send_json(response, api_ok(data), status);
}

void send_error(httplib::Response& response, const std::string& code, const std::string& message, int status) {
  send_json(response, api_error(code, message), status);
}

nlohmann::json parse_body(const httplib::Request& request) {
  if (request.body.empty()) {
    return nlohmann::json::object();
  }

  auto parsed = nlohmann::json::parse(request.body, nullptr, false);
  return parsed.is_discarded() ? nlohmann::json::object() : parsed;
}

bool is_loopback_request(const httplib::Request& request) {
  return request.remote_addr == "127.0.0.1" || request.remote_addr == "::1" ||
         request.remote_addr.rfind("::ffff:127.", 0) == 0 || request.remote_addr.empty();
}

}  // namespace app
