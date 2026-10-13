#pragma once

#include "core/threadbus.hpp"
#include "net/protocol.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <utility>

namespace net {

namespace detail {

struct LinkState {
  ChannelID         toServer = 0;
  ChannelID         toClient = 0;
  std::atomic<bool> open     = true;

  LinkState() = default;
  ~LinkState() {
    THREAD_BUS.unregisterChannel(toServer);
    THREAD_BUS.unregisterChannel(toClient);
  }

  LinkState(const LinkState&)            = delete;
  LinkState(LinkState&&)                 = delete;
  LinkState& operator=(const LinkState&) = delete;
  LinkState& operator=(LinkState&&)      = delete;
};

}

template <typename Out, typename In>
class Link {
  std::shared_ptr<detail::LinkState> m_state;
  ChannelID                          m_send   = 0;
  ChannelID                          m_listen = 0;

public:
  Link() = default;

  Link(std::shared_ptr<detail::LinkState> state, ChannelID send, ChannelID listen)
  : m_state(std::move(state)),
    m_send(send),
    m_listen(listen) {}

  ~Link() { close(); }

  Link(const Link&)            = delete;
  Link& operator=(const Link&) = delete;

  Link(Link&& other) noexcept
  : m_state(std::move(other.m_state)),
    m_send(other.m_send),
    m_listen(other.m_listen) {}

  Link& operator=(Link&& other) noexcept {
    if (this != &other) {
      close();
      m_state  = std::move(other.m_state);
      m_send   = other.m_send;
      m_listen = other.m_listen;
    }
    return *this;
  }

  void send(Out message) {
    if (open()) THREAD_BUS.send<Out>(m_send, std::move(message));
  }

  std::optional<In> poll() {
    if (m_state == nullptr) return std::nullopt;
    return THREAD_BUS.receive<In>(m_listen);
  }

  bool open() const {
    return m_state != nullptr && m_state->open.load(std::memory_order_acquire);
  }

  void close() {
    if (m_state != nullptr) m_state->open.store(false, std::memory_order_release);
  }
};

using ClientLink = Link<protocol::ClientMessage, protocol::ServerMessage>;
using ServerLink = Link<protocol::ServerMessage, protocol::ClientMessage>;

inline std::pair<ClientLink, ServerLink> makeLinkPair() {
  auto state      = std::make_shared<detail::LinkState>();
  state->toServer = THREAD_BUS.registerChannel<protocol::ClientMessage>();
  state->toClient = THREAD_BUS.registerChannel<protocol::ServerMessage>();

  const ChannelID toServer = state->toServer;
  const ChannelID toClient = state->toClient;

  return {
    ClientLink(state, toServer, toClient),
    ServerLink(std::move(state), toClient, toServer)
  };
}

}
