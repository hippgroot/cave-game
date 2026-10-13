#include "net/enet.hpp"

#include "core/log.hpp"

#include <array>
#include <cstdlib>
#include <format>
#include <mutex>

namespace net::enet {

bool initialize() {
  static std::once_flag once;
  static bool           initialized = false;

  std::call_once(once, [] {
    initialized = enet_initialize() == 0;
    if (!initialized) {
      logging::net.error("Failed to initialize ENet");
      return;
    }

    std::atexit(enet_deinitialize);
    logging::net.debug("ENet {}.{}.{} initialized", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
  });

  return initialized;
}

str toString(const ENetAddress& address) {
  std::array<char, 64> host{};
  if (enet_address_get_host_ip(&address, host.data(), host.size()) != 0) return std::format("?:{}", address.port);
  return std::format("{}:{}", host.data(), address.port);
}

bool send(ENetPeer* peer, std::span<const u8> bytes, bool reliable) {
  ENetPacket* packet = enet_packet_create(bytes.data(), bytes.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
  if (packet == nullptr) return false;

  if (enet_peer_send(peer, reliable ? reliableChannel : unreliableChannel, packet) != 0) {
    if (packet->referenceCount == 0) enet_packet_destroy(packet);
    return false;
  }

  return true;
}

}
