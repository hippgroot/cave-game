#pragma once

#include "core/threadbus.hpp"
#include "core/types.hpp"

#include <variant>

namespace protocol {

inline constexpr u32 currentVersion = 1;

struct Hello {
  u32 version;
  str playerName;
};

struct PlayerInput {
  u32  sequence;
  vec2 move;
  f32  pitch;
  f32  yaw;
  u32  buttons;
};

struct Goodbye {};

struct Welcome {
  u64 playerID;
  u64 tick;
};

struct TickSync {
  u64 tick;
  u32 lastInputSequence;
};

struct PlayerJoined {
  u64 id;
  str name;
};

struct PlayerLeft {
  u64 id;
};

struct Disconnected {
  str reason;
};

using ClientMessage = std::variant<Hello, PlayerInput, Goodbye>;
using ServerMessage = std::variant<Welcome, TickSync, PlayerJoined, PlayerLeft, Disconnected>;

}

class ConnectionStrategy {
protected:
  using Message = std::optional<protocol::ServerMessage>;

public:
  inline virtual ~ConnectionStrategy() = default;

  virtual void    send(protocol::ClientMessage msg) = 0;
  virtual Message poll()                            = 0;
  virtual bool    open() const                      = 0;
};

class LocalConnection : public ConnectionStrategy {
  ChannelID m_sendChannel   = 0;
  ChannelID m_listenChannel = 0;

public:
  inline LocalConnection() {
    m_sendChannel   = THREAD_BUS.registerChannel<protocol::ClientMessage>();
    m_listenChannel = THREAD_BUS.registerChannel<protocol::ServerMessage>();
  }

  inline ~LocalConnection() override {
    THREAD_BUS.unregisterChannel(m_sendChannel);
    THREAD_BUS.unregisterChannel(m_listenChannel);
  }

  inline void send(protocol::ClientMessage msg) override {
    THREAD_BUS.send(m_sendChannel, std::move(msg));
  }

  inline Message poll() override {
    return THREAD_BUS.receive<Message::value_type>(m_listenChannel);
  }

  inline bool open() const override {
    return true;
  }

  inline ChannelID peerListenChannel() const {
    return m_sendChannel;
  }

  inline ChannelID peerSendChannel() const {
    return m_listenChannel;
  }
};

class PeerStrategy {
protected:
  using Message = std::optional<protocol::ClientMessage>;

public:
  inline virtual ~PeerStrategy() = default;

  virtual void    send(protocol::ServerMessage msg) = 0;
  virtual Message poll()                            = 0;
  virtual bool    open() const                      = 0;
};

class LocalPeer : public PeerStrategy {
  ChannelID m_sendChannel   = 0;
  ChannelID m_listenChannel = 0;

public:
  inline void send(protocol::ServerMessage msg) override {
    THREAD_BUS.send(m_sendChannel, std::move(msg));
  }

  inline Message poll() override {
    return THREAD_BUS.receive<Message::value_type>(m_listenChannel);
  }

  inline bool open() const override {
    return true;
  }

  inline void linkSendChannel(ChannelID sendChannel) {
    m_sendChannel = sendChannel;
  }

  inline void linkListenChannel(ChannelID listenChannel) {
    m_listenChannel = listenChannel;
  }
};
