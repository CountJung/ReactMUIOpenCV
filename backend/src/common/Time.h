#pragma once

#include <chrono>
#include <string>

namespace app {

using Clock = std::chrono::system_clock;

std::string to_iso_time(Clock::time_point time_point);

}  // namespace app
