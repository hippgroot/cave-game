#pragma once

#include "core/threadbus.hpp"
#include "game/player/player.hpp"
#include "game/world/chunk.hpp"
#include "game/world/world.hpp"
#include "net/link.hpp"
#include "net/protocol.hpp"

#include <deque>
#include <optional>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <unordered_set>

class Game {
public:
  using PeerID = u64;

  static constexpr u32 tickRate            = 20;
  static constexpr f32 tickDelta           = 1.0f / tickRate;
  static constexpr u32 maxQueuedInputs     = tickRate;
  static constexpr u32 inputBuffer         = 2;
  static constexpr u64 handshakeTimeout    = tickRate * 5;
  static constexpr i32 minViewDistance     = 2;
  static constexpr i32 maxViewDistance     = 32;
  static constexpr i32 defaultViewDistance = 8;
  static constexpr u32 chunksPerTick       = 32;

  struct Info {
    u64 seed = 0;
  };

private:
  struct Peer {
    net::ServerLink                            link;
    std::optional<protocol::PlayerID>          player;
    str                                        name;
    std::optional<Player>                      avatar;
    std::deque<protocol::PlayerInput>          inputs;
    u32                                        lastInputSequence = 0;
    u64                                        connectedTick     = 0;
    bool                                       leaving           = false;
    str                                        leaveReason;
    ChunkPos                                   chunkCenter  = ChunkPos(0);
    i32                                        viewDistance = defaultViewDistance;
    std::unordered_set<ChunkPos, ChunkPosHash> sentChunks;
  };

  World                            m_world;
  ChannelID                        m_joinChannel  = 0;
  u64                              m_tick         = 0;
  PeerID                           m_nextPeerID   = 0;
  protocol::PlayerID               m_nextPlayerID = 0;
  std::unordered_map<PeerID, Peer> m_peers;
  std::jthread                     m_thread;

public:
  explicit Game(const Info& info);
  ~Game();

  Game(const Game&)            = delete;
  Game(Game&&)                 = delete;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&)      = delete;

  ChannelID joinChannel() const;
  void      connect(net::ServerLink link) const;

private:
  void loop(std::stop_token st);
  void tick();

  void acceptPeers();
  void receive();
  void expirePeers();
  void removePeers();
  void simulate();
  void updateWorld();
  void streamChunks();
  void sync();

  void unloadChunks(PeerID id, Peer& peer);
  void loadChunks(PeerID id, Peer& peer);

  void handle(PeerID id, Peer& peer, const protocol::Hello& hello);
  void handle(PeerID id, Peer& peer, const protocol::PlayerInput& input);
  void handle(PeerID id, Peer& peer, const protocol::Goodbye& goodbye);
  void handle(PeerID id, Peer& peer, const protocol::ViewDistance& viewDistance);

  void send(PeerID id, Peer& peer, protocol::ServerMessage message);
  void broadcast(const protocol::ServerMessage& message, std::optional<PeerID> except = std::nullopt);
  void kick(PeerID id, Peer& peer, str reason);
};
