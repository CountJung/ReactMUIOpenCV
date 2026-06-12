#pragma once

#include <nlohmann/json.hpp>

#include <exception>

namespace app {

template <typename T>
T json_value_or(const nlohmann::json& object, const char* key, T fallback) {
  if (!object.is_object() || !object.contains(key)) {
    return fallback;
  }

  try {
    return object.at(key).get<T>();
  } catch (const std::exception&) {
    return fallback;
  }
}

}  // namespace app
