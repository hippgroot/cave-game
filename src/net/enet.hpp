#pragma once

#include "core/types.hpp"

#include <enet/enet.h>

#include <span>

namespace net::enet {

inline constexpr u8  reliableChannel   = 0;
inline constexpr u8  unreliableChannel = 1;
inline constexpr u8  channelCount      = 2;
inline constexpr u32 serviceTimeoutMs  = 2;

bool initialize();
str  toString(const ENetAddress& address);
bool send(ENetPeer* peer, std::span<const u8> bytes, bool reliable);

}
