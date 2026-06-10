#pragma once

#include <cstddef>

namespace app {

constexpr int kDefaultPort = 18730;
constexpr int kDefaultWsPort = 18731;
constexpr int kSessionTimeoutMinutes = 30;
constexpr std::size_t kMaxRecentEvents = 200;
constexpr std::size_t kMaxRecentLogs = 300;
constexpr std::size_t kMaxImageResults = 60;
constexpr std::size_t kMaxApiPayloadBytes = 512ULL * 1024ULL * 1024ULL;
constexpr std::size_t kMaxImageUploadBytes = 25ULL * 1024ULL * 1024ULL;
constexpr std::size_t kMaxVideoUploadBytes = 512ULL * 1024ULL * 1024ULL;

}  // namespace app
