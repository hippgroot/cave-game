#include "net/client.hpp"

#include "core/log.hpp"
#include "net/codec.hpp"
#include "net/enet.hpp"

#include <chrono>
#include <format>

namespace net {

namespace {

constexpr auto _connectTimeout  = std::chrono::seconds(5);
constexpr u32  _connectPumpMs   = 50;
constexpr auto _shutdownTimeout = std::chrono::milliseconds(250);
constexpr u32  _shutdownPumpMs  = 10;

}

Client::Client(Address address, ServerLink link)
: m_address(std::move(address)),
  m_link(std::move(link)) {
  m_thread = std::jthread([this](std::stop_token st) { loop(st); });
}

Client::~Client() = default;

void Client::loop(std::stop_token st) {
  logging::ThreadScope thread("net-client");

  if (connect(st)) {
    while (!st.stop_requested() && m_connected) {
      pump(enet::serviceTimeoutMs);
      forward();
    }
  }

  shutdown();
}

bool Client::connect(std::stop_token st) {
  if (!enet::initialize()) {
    fail("Networking is unavailable");
    return false;
  }

  m_host = enet_host_create(nullptr, 1, enet::channelCount, 0, 0);
  if (m_host == nullptr) {
    fail("Failed to create a network host");
    return false;
  }

  logging::net.info("Connecting to {}", m_address.toString());

  ENetAddress address{};
  if (enet_address_set_host(&address, m_address.host.c_str()) != 0) {
    fail(std::format("Unknown host '{}'", m_address.host));
    return false;
  }
  address.port = m_address.port;

  m_peer = enet_host_connect(m_host, &address, enet::channelCount, 0);
  if (m_peer == nullptr) {
    fail("No free connection slot");
    return false;
  }

  const auto deadline = std::chrono::steady_clock::now() + _connectTimeout;
  while (!st.stop_requested() && std::chrono::steady_clock::now() < deadline) {
    ENetEvent event;
    if (enet_host_service(m_host, &event, _connectPumpMs) <= 0) continue;

    if (event.type == ENET_EVENT_TYPE_CONNECT) {
      m_connected = true;
      logging::net.info("Connected to {}", enet::toString(address));
      return true;
    }

    if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
    if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
  }

  enet_peer_reset(m_peer);
  m_peer = nullptr;

  if (!st.stop_requested()) fail(std::format("Could not connect to {}", m_address.toString()));
  return false;
}

void Client::pump(u32 timeoutMs) {
  ENetEvent event;
  i32       result = enet_host_service(m_host, &event, timeoutMs);

  while (result > 0) {
    switch (event.type) {
    case ENET_EVENT_TYPE_RECEIVE: {
      auto message = decode<protocol::ServerMessage>(std::span<const u8>(event.packet->data, event.packet->dataLength));
      if (!message) {
        logging::net.warn("Malformed packet ({} bytes) from server, ignoring", event.packet->dataLength);
      } else {
        logging::net.trace("<- server {}", protocol::nameOf(*message));
        m_link.send(std::move(*message));
      }
      enet_packet_destroy(event.packet);
      break;
    }

    case ENET_EVENT_TYPE_DISCONNECT:
      logging::net.info("Disconnected from {}", m_address.toString());
      m_connected = false;
      m_peer      = nullptr;
      m_link.close();
      break;

    case ENET_EVENT_TYPE_CONNECT:
    case ENET_EVENT_TYPE_NONE:    break;
    }

    if (!m_connected) break;
    result = enet_host_check_events(m_host, &event);
  }

  if (result < 0) logging::net.error("ENet host service failed");
}

void Client::forward() {
  if (!m_connected || m_disconnecting) return;

  const bool closed = !m_link.open();
  while (auto message = m_link.poll()) {
    logging::net.trace("-> server {}", protocol::nameOf(*message));
    if (!enet::send(m_peer, encode(*message), protocol::isReliable(*message)))
      logging::net.warn("Failed to send {} to server", protocol::nameOf(*message));
  }

  if (closed) {
    logging::net.debug("Disconnecting from {}", m_address.toString());
    enet_peer_disconnect_later(m_peer, 0);
    m_disconnecting = true;
  }

  enet_host_flush(m_host);
}

void Client::shutdown() {
  if (m_connected) {
    m_link.close();
    forward();

    const auto deadline = std::chrono::steady_clock::now() + _shutdownTimeout;
    while (m_connected && std::chrono::steady_clock::now() < deadline)
      pump(_shutdownPumpMs);

    if (m_connected) {
      logging::net.debug("Server did not acknowledge disconnect, dropping");
      enet_peer_reset(m_peer);
      m_connected = false;
      m_peer      = nullptr;
    }
  }

  m_link.close();

  if (m_host != nullptr) {
    enet_host_destroy(m_host);
    m_host = nullptr;
  }
}

void Client::fail(std::string_view reason) {
  logging::net.warn("{}", reason);
  m_link.send(protocol::Disconnected{.reason = str(reason)});
  m_link.close();
}

}
