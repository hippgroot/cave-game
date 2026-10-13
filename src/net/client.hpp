#pragma once

#include "net/address.hpp"
#include "net/link.hpp"

#include <stop_token>
#include <string_view>
#include <thread>

struct _ENetHost;
struct _ENetPeer;

namespace net {

class Client {
  Address      m_address;
  ServerLink   m_link;
  _ENetHost*   m_host          = nullptr;
  _ENetPeer*   m_peer          = nullptr;
  bool         m_connected     = false;
  bool         m_disconnecting = false;
  std::jthread m_thread;

public:
  Client(Address address, ServerLink link);
  ~Client();

  Client(const Client&)            = delete;
  Client(Client&&)                 = delete;
  Client& operator=(const Client&) = delete;
  Client& operator=(Client&&)      = delete;

private:
  void loop(std::stop_token st);
  bool connect(std::stop_token st);
  void pump(u32 timeoutMs);
  void forward();
  void shutdown();
  void fail(std::string_view reason);
};

}
