#pragma once

#include "core/threadbus.hpp"
#include "net/link.hpp"
#include "net/protocol.hpp"

#include <optional>
#include <stop_token>
#include <thread>
#include <unordered_map>

class Game {
public:
  using PeerID = u64;

  static constexpr u32 tickRate         = 20;
  static constexpr u64 handshakeTimeout = tickRate * 5;

private:
  struct Peer {
    net::ServerLink                   link;
    std::optional<protocol::PlayerID> player;
    str                               name;
    protocol::PlayerInput             input;
    u64                               connectedTick = 0;
    bool                              leaving       = false;
    str                               leaveReason;
  };

  ChannelID                        m_joinChannel  = 0;
  u64                              m_tick         = 0;
  PeerID                           m_nextPeerID   = 0;
  protocol::PlayerID               m_nextPlayerID = 0;
  std::unordered_map<PeerID, Peer> m_peers;
  std::jthread                     m_thread;

public:
  Game();
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
  void sync();

  void handle(PeerID id, Peer& peer, const protocol::Hello& hello);
  void handle(PeerID id, Peer& peer, const protocol::PlayerInput& input);
  void handle(PeerID id, Peer& peer, const protocol::Goodbye& goodbye);

  void send(PeerID id, Peer& peer, protocol::ServerMessage message);
  void broadcast(const protocol::ServerMessage& message, std::optional<PeerID> except = std::nullopt);
  void kick(PeerID id, Peer& peer, str reason);
};
