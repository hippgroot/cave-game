#pragma once

#include "client/client_world.hpp"
#include "client/player_prediction.hpp"
#include "game/player/controls.hpp"
#include "net/address.hpp"
#include "net/link.hpp"
#include "net/protocol.hpp"

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>

class Game;

namespace net {
class Client;
class Server;
}

class Session {
public:
  enum class Event : u8 {
    None,
    Joined,
    Ended
  };

private:
  net::ClientLink              m_link;
  std::unique_ptr<net::Client> m_remote;
  std::unique_ptr<net::Server> m_lan;
  std::unique_ptr<Game>        m_game;

  std::optional<protocol::PlayerID>           m_playerID;
  std::unordered_map<protocol::PlayerID, str> m_players;
  u64                                         m_serverTick = 0;
  bool                                        m_ended      = false;
  str                                         m_endReason;
  std::optional<i32>                          m_viewDistance;
  ClientWorld                                 m_world;
  std::optional<PlayerPrediction>             m_prediction;

public:
  static std::unique_ptr<Session> singleplayer(u64 seed, std::string_view playerName);
  static std::unique_ptr<Session> join(const net::Address& address, std::string_view playerName);
  ~Session();

  Session(const Session&)            = delete;
  Session(Session&&)                 = delete;
  Session& operator=(const Session&) = delete;
  Session& operator=(Session&&)      = delete;

  Event update(f32 dt, const Controls& controls);

  bool               inGame() const;
  bool               hosting() const;
  std::optional<u16> lanPort() const;
  bool               openToLan(u16 port = net::defaultPort);
  void               setViewDistance(i32 chunks);

  const ClientWorld&      world() const;
  const PlayerPrediction* player() const;
  std::vector<ChunkEvent> takeChunkEvents();

  const str& endReason() const;

private:
  explicit Session(net::ClientLink link);

  void hello(std::string_view playerName);
  void end(str reason);
};
