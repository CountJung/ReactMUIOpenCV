#include "Random.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <bcrypt.h>
#endif

namespace app {

namespace {

std::vector<std::uint8_t> random_bytes(std::size_t length) {
  std::vector<std::uint8_t> bytes(length);
  if (bytes.empty()) {
    return bytes;
  }

#ifdef _WIN32
  const auto status =
      BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status < 0) {
    throw std::runtime_error("BCryptGenRandom failed.");
  }
#else
  std::random_device random_device;
  for (auto& byte : bytes) {
    byte = static_cast<std::uint8_t>(random_device());
  }
#endif

  return bytes;
}

}  // namespace

std::string random_hex(std::size_t length) {
  static constexpr std::array<char, 16> kHex = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  const auto bytes = random_bytes(length);

  std::string value;
  value.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    value.push_back(kHex[bytes[i] & 0x0f]);
  }

  return value;
}

std::string random_pin() {
  const auto bytes = random_bytes(4);
  const auto value = (static_cast<unsigned int>(bytes[0]) << 24) | (static_cast<unsigned int>(bytes[1]) << 16) |
                     (static_cast<unsigned int>(bytes[2]) << 8) | static_cast<unsigned int>(bytes[3]);

  std::ostringstream stream;
  stream << std::setw(6) << std::setfill('0') << (value % 1000000);
  return stream.str();
}

}  // namespace app
