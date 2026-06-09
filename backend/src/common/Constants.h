#pragma once

#include <cstddef>

namespace app {

constexpr int kDefaultPort = 18730;
constexpr int kDefaultWsPort = 18731;
constexpr int kSessionTimeoutMinutes = 30;
constexpr std::size_t kMaxRecentEvents = 200;
constexpr std::size_t kMaxRecentLogs = 300;
constexpr std::size_t kMaxImageResults = 60;

}  // namespace app
