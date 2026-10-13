#include "game/game.hpp"

#include "core/network.hpp"
#include "core/overloaded.hpp"

#include <chrono>
#include <thread>

Game::Game() {
  m_joinChannel = THREAD_BUS.registerChannel<std::unique_ptr<PeerStrategy>>();
  m_thread      = std::jthread([this](std::stop_token st) { loop(st); });
}

Game::~Game() {
  THREAD_BUS.unregisterChannel(m_joinChannel);
}

ChannelID Game::joinChannel() const {
  return m_joinChannel;
}

void Game::loop(std::stop_token st) {
  using clock          = std::chrono::steady_clock;
  constexpr auto _tick = std::chrono::milliseconds(50);

  auto next = clock::now();
  while (!st.stop_requested()) {
    next += _tick;

    acceptPeers();
    listen();

    ++m_tick;

    auto now = clock::now();
    if (now - next > 10 * _tick) next = now;
    std::this_thread::sleep_until(next);
  }
}

void Game::acceptPeers() {
  while (auto peeropt = THREAD_BUS.receive<std::unique_ptr<PeerStrategy>>(m_joinChannel)) {
    m_peers.emplace(++m_nextPeerID, std::move(*peeropt));
  }
}

void Game::listen() {
  using namespace protocol;

  for (auto& [id, peer] : m_peers) {
    while (auto msgopt = peer->poll()) {
      std::visit(overloaded{
                   [&](const Hello& hello) {
                     peer->send(Welcome{
                       .playerID = id,
                       .tick     = m_tick
                     });
                   },
                   [&](const PlayerInput& playerInput) {},
                   [&](const Goodbye& goodbye) {}
                 },
        std::move(*msgopt));
    }
  }
}
