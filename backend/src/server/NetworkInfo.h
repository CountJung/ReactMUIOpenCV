#pragma once

#include <string>
#include <vector>

namespace app {

bool is_private_ipv4(const std::string& ip);
std::vector<std::string> get_lan_ipv4_addresses();

}  // namespace app
