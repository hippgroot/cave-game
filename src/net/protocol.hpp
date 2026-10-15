#pragma once

#include "core/types.hpp"

#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace protocol {

using PlayerID = u64;

inline constexpr u32  version             = 2;
inline constexpr size maxPlayerNameLength = 16;
inline constexpr size maxReasonLength     = 256;

struct Hello {
  static constexpr std::string_view name = "Hello";

  u32 version = 0;
  str playerName;

  auto fields(this auto& self) { return std::tie(self.version, self.playerName); }
};

struct PlayerInput {
  static constexpr std::string_view name = "PlayerInput";

  u32  sequence = 0;
  vec2 move     = vec2(0.0f);
  f32  pitch    = 0.0f;
  f32  yaw      = 0.0f;
  u32  buttons  = 0;

  auto fields(this auto& self) { return std::tie(self.sequence, self.move, self.pitch, self.yaw, self.buttons); }
};

struct Goodbye {
  static constexpr std::string_view name = "Goodbye";

  auto fields(this auto&) { return std::tie(); }
};

struct ViewDistance {
  static constexpr std::string_view name = "ViewDistance";

  u32 chunks = 0;

  auto fields(this auto& self) { return std::tie(self.chunks); }
};

struct Welcome {
  static constexpr std::string_view name = "Welcome";

  PlayerID playerID = 0;
  u64      tick     = 0;

  auto fields(this auto& self) { return std::tie(self.playerID, self.tick); }
};

struct TickSync {
  static constexpr std::string_view name = "TickSync";

  u64 tick              = 0;
  u32 lastInputSequence = 0;

  auto fields(this auto& self) { return std::tie(self.tick, self.lastInputSequence); }
};

struct PlayerJoined {
  static constexpr std::string_view name = "PlayerJoined";

  PlayerID id = 0;
  str      playerName;

  auto fields(this auto& self) { return std::tie(self.id, self.playerName); }
};

struct PlayerLeft {
  static constexpr std::string_view name = "PlayerLeft";

  PlayerID id = 0;

  auto fields(this auto& self) { return std::tie(self.id); }
};

struct Disconnected {
  static constexpr std::string_view name = "Disconnected";

  str reason;

  auto fields(this auto& self) { return std::tie(self.reason); }
};

struct ChunkData {
  static constexpr std::string_view name = "ChunkData";

  ivec2           position = ivec2(0);
  std::vector<u8> blocks;

  auto fields(this auto& self) { return std::tie(self.position, self.blocks); }
};

struct ChunkUnload {
  static constexpr std::string_view name = "ChunkUnload";

  ivec2 position = ivec2(0);

  auto fields(this auto& self) { return std::tie(self.position); }
};

struct PlayerSync {
  static constexpr std::string_view name = "PlayerSync";

  u32  lastInputSequence = 0;
  u8   mode              = 0;
  vec3 position          = vec3(0.0f);
  vec3 velocity          = vec3(0.0f);

  auto fields(this auto& self) { return std::tie(self.lastInputSequence, self.mode, self.position, self.velocity); }
};

using ClientMessage = std::variant<Hello, PlayerInput, Goodbye, ViewDistance>;
using ServerMessage = std::variant<Welcome, TickSync, PlayerJoined, PlayerLeft, Disconnected, ChunkData, ChunkUnload, PlayerSync>;

template <typename Message>
std::string_view nameOf(const Message& message) {
  return std::visit([](const auto& alternative) { return std::remove_cvref_t<decltype(alternative)>::name; }, message);
}

template <typename Message>
bool isReliable(const Message& message) {
  return std::visit([]<typename T>(const T&) {
    return !std::is_same_v<T, PlayerInput> && !std::is_same_v<T, TickSync> && !std::is_same_v<T, PlayerSync>;
  },
    message);
}

}
