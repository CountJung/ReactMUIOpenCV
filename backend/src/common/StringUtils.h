#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>

namespace app {

std::string lowercase_copy(std::string value);
std::string sanitize_file_stem(std::string value, std::string fallback = "file");
bool has_supported_extension(const std::filesystem::path& path, std::initializer_list<std::string_view> extensions);
std::filesystem::path path_from_utf8(std::string_view value);
std::string path_to_utf8(const std::filesystem::path& path);

}  // namespace app
