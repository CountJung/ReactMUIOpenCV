#include "StringUtils.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <utility>

namespace app {

std::string lowercase_copy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

std::string sanitize_file_stem(std::string value, std::string fallback) {
  for (auto& character : value) {
    const auto byte = static_cast<unsigned char>(character);
    if (!std::isalnum(byte) && character != '-' && character != '_') {
      character = '_';
    }
  }

  return value.empty() ? std::move(fallback) : value;
}

bool has_supported_extension(const std::filesystem::path& path, std::initializer_list<std::string_view> extensions) {
  const auto extension = lowercase_copy(path.extension().string());
  return std::any_of(
      extensions.begin(), extensions.end(), [&](std::string_view supported) { return extension == supported; });
}

std::filesystem::path path_from_utf8(std::string_view value) {
  std::u8string utf8;
  utf8.reserve(value.size());
  std::transform(value.begin(), value.end(), std::back_inserter(utf8), [](char character) {
    return static_cast<char8_t>(static_cast<unsigned char>(character));
  });
  return std::filesystem::path(utf8);
}

std::string path_to_utf8(const std::filesystem::path& path) {
  const auto utf8 = path.generic_u8string();
  return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

}  // namespace app
