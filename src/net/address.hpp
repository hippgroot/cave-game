#pragma once

#include "core/types.hpp"

#include <charconv>
#include <format>
#include <optional>
#include <string_view>

namespace net {

inline constexpr u16 defaultPort = 7777;

struct Address {
  str host;
  u16 port = defaultPort;

  static std::optional<Address> parse(std::string_view text) {
    while (!text.empty() && text.front() == ' ')
      text.remove_prefix(1);
    while (!text.empty() && text.back() == ' ')
      text.remove_suffix(1);
    if (text.empty()) return std::nullopt;

    const auto colon = text.rfind(':');
    if (colon == std::string_view::npos) return Address{.host = str(text), .port = defaultPort};

    const std::string_view host = text.substr(0, colon);
    const std::string_view port = text.substr(colon + 1);
    if (host.empty() || port.empty()) return std::nullopt;

    u16 value               = 0;
    const auto [end, error] = std::from_chars(port.data(), port.data() + port.size(), value);
    if (error != std::errc() || end != port.data() + port.size() || value == 0) return std::nullopt;

    return Address{.host = str(host), .port = value};
  }

  str toString() const { return std::format("{}:{}", host, port); }
};

}
