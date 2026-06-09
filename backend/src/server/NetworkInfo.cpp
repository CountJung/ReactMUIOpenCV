#include "NetworkInfo.h"

#include <algorithm>
#include <set>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace app {

bool is_private_ipv4(const std::string& ip) {
  return ip.rfind("10.", 0) == 0 || ip.rfind("192.168.", 0) == 0 ||
         ip.rfind("172.16.", 0) == 0 || ip.rfind("172.17.", 0) == 0 ||
         ip.rfind("172.18.", 0) == 0 || ip.rfind("172.19.", 0) == 0 ||
         ip.rfind("172.20.", 0) == 0 || ip.rfind("172.21.", 0) == 0 ||
         ip.rfind("172.22.", 0) == 0 || ip.rfind("172.23.", 0) == 0 ||
         ip.rfind("172.24.", 0) == 0 || ip.rfind("172.25.", 0) == 0 ||
         ip.rfind("172.26.", 0) == 0 || ip.rfind("172.27.", 0) == 0 ||
         ip.rfind("172.28.", 0) == 0 || ip.rfind("172.29.", 0) == 0 ||
         ip.rfind("172.30.", 0) == 0 || ip.rfind("172.31.", 0) == 0;
}

std::vector<std::string> get_lan_ipv4_addresses() {
#ifdef _WIN32
  WSADATA wsa_data{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return {};
  }
#endif

  char hostname[256]{};
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    return {};
  }

  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
    return {};
  }

  std::set<std::string> addresses;
  for (addrinfo* current = result; current != nullptr; current = current->ai_next) {
    char buffer[INET_ADDRSTRLEN]{};
    auto* ipv4 = reinterpret_cast<sockaddr_in*>(current->ai_addr);
    if (inet_ntop(AF_INET, &(ipv4->sin_addr), buffer, sizeof(buffer)) != nullptr) {
      const std::string ip = buffer;
      if (ip != "127.0.0.1" && ip.rfind("169.254.", 0) != 0) {
        addresses.insert(ip);
      }
    }
  }

  freeaddrinfo(result);

  std::vector<std::string> sorted(addresses.begin(), addresses.end());
  std::stable_sort(sorted.begin(), sorted.end(), [](const std::string& left, const std::string& right) {
    return is_private_ipv4(left) > is_private_ipv4(right);
  });

  return sorted;
}

}  // namespace app
