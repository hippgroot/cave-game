#pragma once

#include "core/threadbus.hpp"
#include "net/address.hpp"
#include "net/link.hpp"

#include <memory>
#include <stop_token>
#include <thread>
#include <unordered_map>

struct _ENetHost;
struct _ENetPeer;

namespace net {

class Server {
public:
  struct Info {
    u16       port        = defaultPort;
    ChannelID joinChannel = 0;
    size      maxPeers    = 32;
  };

private:
  struct Connection {
    _ENetPeer* peer = nullptr;
    ClientLink link;
    str        address;
    bool       disconnecting = false;
  };

  _ENetHost*                          m_host        = nullptr;
  ChannelID                           m_joinChannel = 0;
  u16                                 m_port        = 0;
  u64                                 m_nextID      = 0;
  bool                                m_closing     = false;
  std::unordered_map<u64, Connection> m_connections;
  std::jthread                        m_thread;

public:
  static std::unique_ptr<Server> listen(const Info& info);
  ~Server();

  Server(const Server&)            = delete;
  Server(Server&&)                 = delete;
  Server& operator=(const Server&) = delete;
  Server& operator=(Server&&)      = delete;

  u16 port() const { return m_port; }

private:
  Server(_ENetHost* host, const Info& info);

  void loop(std::stop_token st);
  void pump(u32 timeoutMs);
  void forward();
  void shutdown();

  Connection* find(_ENetPeer* peer);
};

}
