#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <string>

namespace app {

nlohmann::json api_ok(const nlohmann::json& data = nlohmann::json::object());
nlohmann::json api_error(const std::string& code, const std::string& message);
void set_cors(httplib::Response& response);
void send_json(httplib::Response& response, const nlohmann::json& body, int status = 200);
void send_data(httplib::Response& response, const nlohmann::json& data, int status = 200);
void send_error(httplib::Response& response, const std::string& code, const std::string& message, int status);
nlohmann::json parse_body(const httplib::Request& request);
bool is_loopback_request(const httplib::Request& request);

}  // namespace app
