#include "game/game.hpp"

#include "core/log.hpp"

#include <chrono>
#include <format>
#include <vector>

namespace {

constexpr auto _tickDuration  = std::chrono::nanoseconds(std::chrono::seconds(1)) / Game::tickRate;
constexpr u32  _maxCatchUp    = 10;
constexpr auto _closedMessage = "Server closed";

str sanitizeName(std::string_view name) {
  str result;
  for (const char c : name) {
    if (result.size() >= protocol::maxPlayerNameLength) break;
    if (c >= ' ' && c <= '~') result.push_back(c);
  }
  return result.empty() ? str("Player") : result;
}

}

Game::Game()
: m_joinChannel(THREAD_BUS.registerChannel<net::ServerLink>()) {
  m_thread = std::jthread([this](std::stop_token st) { loop(st); });
}

Game::~Game() {
  m_thread.request_stop();
  if (m_thread.joinable()) m_thread.join();

  for (auto& [id, peer] : m_peers) {
    peer.link.send(protocol::Disconnected{.reason = _closedMessage});
    peer.link.close();
  }
  m_peers.clear();

  THREAD_BUS.unregisterChannel(m_joinChannel);
}

ChannelID Game::joinChannel() const {
  return m_joinChannel;
}

void Game::connect(net::ServerLink link) const {
  THREAD_BUS.send<net::ServerLink>(m_joinChannel, std::move(link));
}

void Game::loop(std::stop_token st) {
  logging::ThreadScope thread("game");
  logging::game.info("Running at {} ticks per second", tickRate);

  using Clock = std::chrono::steady_clock;

  auto next = Clock::now();
  while (!st.stop_requested()) {
    next += _tickDuration;

    tick();

    const auto now = Clock::now();
    if (now - next > _tickDuration * _maxCatchUp) {
      const auto behind = std::chrono::duration_cast<std::chrono::milliseconds>(now - next);
      logging::game.warn("Can't keep up! Running {}ms behind, skipping {} ticks", behind.count(), behind / _tickDuration);
      next = now;
    }

    std::this_thread::sleep_until(next);
  }

  logging::game.info("Stopped at tick {}", m_tick);
}

void Game::tick() {
  acceptPeers();
  receive();
  expirePeers();
  removePeers();
  simulate();
  sync();
  ++m_tick;
}

void Game::acceptPeers() {
  while (auto link = THREAD_BUS.receive<net::ServerLink>(m_joinChannel)) {
    const PeerID id = ++m_nextPeerID;
    m_peers.try_emplace(id, Peer{.link = std::move(*link), .connectedTick = m_tick});
    logging::game.debug("Peer {} connected, awaiting Hello", id);
  }
}

void Game::receive() {
  for (auto& [id, peer] : m_peers) {
    const bool closed = !peer.link.open();

    while (auto message = peer.link.poll()) {
      logging::net.trace("peer {} -> {}", id, protocol::nameOf(*message));
      std::visit([&](const auto& alternative) { handle(id, peer, alternative); }, *message);
    }

    if (closed && !peer.leaving) {
      peer.leaving     = true;
      peer.leaveReason = "connection lost";
    }
  }
}

void Game::expirePeers() {
  for (auto& [id, peer] : m_peers) {
    if (peer.player || peer.leaving) continue;
    if (m_tick - peer.connectedTick < handshakeTimeout) continue;
    kick(id, peer, "Handshake timed out");
  }
}

void Game::removePeers() {
  std::vector<std::pair<protocol::PlayerID, str>> left;

  std::erase_if(m_peers, [&](auto& entry) {
    auto& [id, peer] = entry;
    if (!peer.leaving) return false;

    if (peer.player) {
      logging::game.info("{} left the game ({})", peer.name, peer.leaveReason);
      left.emplace_back(*peer.player, peer.name);
    } else {
      logging::game.debug("Peer {} dropped before joining ({})", id, peer.leaveReason);
    }

    peer.link.close();
    return true;
  });

  for (const auto& [player, name] : left)
    broadcast(protocol::PlayerLeft{.id = player});
}

void Game::simulate() {}

void Game::sync() {
  for (auto& [id, peer] : m_peers) {
    if (!peer.player || peer.leaving) continue;
    send(id, peer, protocol::TickSync{.tick = m_tick, .lastInputSequence = peer.input.sequence});
  }
}

void Game::handle(PeerID id, Peer& peer, const protocol::Hello& hello) {
  if (peer.player) {
    logging::net.warn("Peer {} sent Hello after joining, ignoring", id);
    return;
  }

  if (hello.version != protocol::version) {
    kick(id, peer, std::format("Incompatible version (server {}, client {})", protocol::version, hello.version));
    return;
  }

  peer.player = ++m_nextPlayerID;
  peer.name   = sanitizeName(hello.playerName);

  send(id, peer, protocol::Welcome{.playerID = *peer.player, .tick = m_tick});

  for (auto& [otherID, other] : m_peers) {
    if (otherID == id || !other.player || other.leaving) continue;
    send(id, peer, protocol::PlayerJoined{.id = *other.player, .playerName = other.name});
  }

  broadcast(protocol::PlayerJoined{.id = *peer.player, .playerName = peer.name}, id);

  logging::game.info("{} joined the game (player {}, peer {})", peer.name, *peer.player, id);
}

void Game::handle(PeerID id, Peer& peer, const protocol::PlayerInput& input) {
  if (!peer.player) {
    logging::net.debug("Ignoring input from peer {} before Hello", id);
    return;
  }

  if (input.sequence <= peer.input.sequence) return;
  peer.input = input;
}

void Game::handle(PeerID id, Peer& peer, const protocol::Goodbye&) {
  logging::net.debug("Peer {} said goodbye", id);
  peer.leaving     = true;
  peer.leaveReason = "quit";
}

void Game::send(PeerID id, Peer& peer, protocol::ServerMessage message) {
  logging::net.trace("peer {} <- {}", id, protocol::nameOf(message));
  peer.link.send(std::move(message));
}

void Game::broadcast(const protocol::ServerMessage& message, std::optional<PeerID> except) {
  for (auto& [id, peer] : m_peers) {
    if (id == except || !peer.player || peer.leaving) continue;
    send(id, peer, message);
  }
}

void Game::kick(PeerID id, Peer& peer, str reason) {
  logging::game.info("Kicking peer {}: {}", id, reason);
  send(id, peer, protocol::Disconnected{.reason = reason});
  peer.leaving     = true;
  peer.leaveReason = std::move(reason);
}
