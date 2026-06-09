#include "Time.h"

#include <iomanip>
#include <sstream>

namespace app {

std::string to_iso_time(Clock::time_point time_point) {
  const auto time = Clock::to_time_t(time_point);
  std::tm utc{};

#ifdef _WIN32
  gmtime_s(&utc, &time);
#else
  gmtime_r(&time, &utc);
#endif

  std::ostringstream stream;
  stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  return stream.str();
}

}  // namespace app
