#include "net/server.hpp"

#include "core/log.hpp"
#include "net/codec.hpp"
#include "net/enet.hpp"

#include <chrono>
#include <cstdint>

namespace net {

namespace {

constexpr auto _shutdownTimeout = std::chrono::milliseconds(250);
constexpr u32  _shutdownPumpMs  = 10;

u64 idOf(const ENetPeer* peer) {
  return static_cast<u64>(reinterpret_cast<std::uintptr_t>(peer->data));
}

}

std::unique_ptr<Server> Server::listen(const Info& info) {
  if (!enet::initialize()) return nullptr;

  ENetAddress address{};
  address.host = ENET_HOST_ANY;
  address.port = info.port;

  ENetHost* host = enet_host_create(&address, info.maxPeers, enet::channelCount, 0, 0);
  if (host == nullptr) {
    logging::net.error("Failed to listen on port {} (is it already in use?)", info.port);
    return nullptr;
  }

  return std::unique_ptr<Server>(new Server(host, info));
}

Server::Server(ENetHost* host, const Info& info)
: m_host(host),
  m_joinChannel(info.joinChannel),
  m_port(info.port) {
  logging::net.info("Listening on port {}", m_port);
  m_thread = std::jthread([this](std::stop_token st) { loop(st); });
}

Server::~Server() = default;

void Server::loop(std::stop_token st) {
  logging::ThreadScope thread("net-server");

  while (!st.stop_requested()) {
    pump(enet::serviceTimeoutMs);
    forward();
  }

  shutdown();
}

void Server::pump(u32 timeoutMs) {
  ENetEvent event;
  i32       result = enet_host_service(m_host, &event, timeoutMs);

  while (result > 0) {
    switch (event.type) {
    case ENET_EVENT_TYPE_CONNECT: {
      if (m_closing) {
        enet_peer_disconnect_now(event.peer, 0);
        break;
      }

      const u64 id     = ++m_nextID;
      event.peer->data = reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));

      auto [client, server] = makeLinkPair();
      const str address     = enet::toString(event.peer->address);
      m_connections.try_emplace(id, Connection{.peer = event.peer, .link = std::move(client), .address = address});
      THREAD_BUS.send<ServerLink>(m_joinChannel, std::move(server));

      logging::net.info("{} connected", address);
      break;
    }

    case ENET_EVENT_TYPE_RECEIVE: {
      if (Connection* connection = find(event.peer)) {
        auto message = decode<protocol::ClientMessage>(std::span<const u8>(event.packet->data, event.packet->dataLength));
        if (!message) {
          logging::net.warn("Malformed packet ({} bytes) from {}, disconnecting", event.packet->dataLength, connection->address);
          connection->link.close();
        } else {
          logging::net.trace("<- {} {}", connection->address, protocol::nameOf(*message));
          connection->link.send(std::move(*message));
        }
      }
      enet_packet_destroy(event.packet);
      break;
    }

    case ENET_EVENT_TYPE_DISCONNECT: {
      if (Connection* connection = find(event.peer)) {
        logging::net.info("{} disconnected", connection->address);
        connection->link.close();
        m_connections.erase(idOf(event.peer));
      }
      event.peer->data = nullptr;
      break;
    }

    case ENET_EVENT_TYPE_NONE: break;
    }

    result = enet_host_check_events(m_host, &event);
  }

  if (result < 0) logging::net.error("ENet host service failed");
}

void Server::forward() {
  for (auto& [id, connection] : m_connections) {
    if (connection.disconnecting) continue;

    const bool closed = !connection.link.open();
    while (auto message = connection.link.poll()) {
      logging::net.trace("-> {} {}", connection.address, protocol::nameOf(*message));
      if (!enet::send(connection.peer, encode(*message), protocol::isReliable(*message)))
        logging::net.warn("Failed to send {} to {}", protocol::nameOf(*message), connection.address);
    }

    if (closed) {
      logging::net.debug("Disconnecting {}", connection.address);
      enet_peer_disconnect_later(connection.peer, 0);
      connection.disconnecting = true;
    }
  }

  enet_host_flush(m_host);
}

void Server::shutdown() {
  m_closing = true;
  forward();

  for (auto& [id, connection] : m_connections) {
    if (connection.disconnecting) continue;
    enet_peer_disconnect_later(connection.peer, 0);
    connection.disconnecting = true;
  }
  enet_host_flush(m_host);

  const auto deadline = std::chrono::steady_clock::now() + _shutdownTimeout;
  while (!m_connections.empty() && std::chrono::steady_clock::now() < deadline)
    pump(_shutdownPumpMs);

  for (auto& [id, connection] : m_connections) {
    logging::net.debug("{} did not acknowledge disconnect, dropping", connection.address);
    enet_peer_reset(connection.peer);
  }
  m_connections.clear();

  enet_host_destroy(m_host);
  m_host = nullptr;

  logging::net.info("Stopped listening on port {}", m_port);
}

Server::Connection* Server::find(ENetPeer* peer) {
  if (peer->data == nullptr) return nullptr;
  auto itr = m_connections.find(idOf(peer));
  return itr == m_connections.end() ? nullptr : &itr->second;
}

}
