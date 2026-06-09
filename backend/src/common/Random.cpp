#include "Random.h"

#include <array>
#include <iomanip>
#include <random>
#include <sstream>

namespace app {

std::string random_hex(std::size_t length) {
  static constexpr std::array<char, 16> kHex = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  static std::random_device random_device;
  static std::mt19937 generator(random_device());
  static std::uniform_int_distribution<int> distribution(0, 15);

  std::string value;
  value.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    value.push_back(kHex[distribution(generator)]);
  }

  return value;
}

std::string random_pin() {
  static std::random_device random_device;
  static std::mt19937 generator(random_device());
  static std::uniform_int_distribution<int> distribution(0, 999999);

  std::ostringstream stream;
  stream << std::setw(6) << std::setfill('0') << distribution(generator);
  return stream.str();
}

}  // namespace app
