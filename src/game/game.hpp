#pragma once

#include "core/network.hpp"

#include <stop_token>
#include <thread>

using PeerID = u64;

class Game {
  ChannelID                                                 m_joinChannel;
  u64                                                       m_tick       = 0;
  PeerID                                                    m_nextPeerID = 0;
  std::unordered_map<PeerID, std::unique_ptr<PeerStrategy>> m_peers;
  std::jthread                                              m_thread;

public:
  Game();
  ~Game();

  Game(const Game&)            = delete;
  Game(Game&&)                 = delete;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&)      = delete;

  ChannelID joinChannel() const;

private:
  void loop(std::stop_token);
  void acceptPeers();
  void listen();
};
