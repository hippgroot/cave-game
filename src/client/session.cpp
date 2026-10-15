#include "client/session.hpp"

#include "core/log.hpp"
#include "core/overloaded.hpp"
#include "game/game.hpp"
#include "net/client.hpp"
#include "net/server.hpp"

std::unique_ptr<Session> Session::singleplayer(u64 seed, std::string_view playerName) {
  auto [client, server] = net::makeLinkPair();
  auto session          = std::unique_ptr<Session>(new Session(std::move(client)));

  session->m_game = std::make_unique<Game>(Game::Info{.seed = seed});
  session->m_game->connect(std::move(server));
  session->hello(playerName);

  return session;
}

std::unique_ptr<Session> Session::join(const net::Address& address, std::string_view playerName) {
  auto [client, server] = net::makeLinkPair();
  auto session          = std::unique_ptr<Session>(new Session(std::move(client)));

  session->m_remote = std::make_unique<net::Client>(address, std::move(server));
  session->hello(playerName);

  return session;
}

Session::Session(net::ClientLink link)
: m_link(std::move(link)) {}

Session::~Session() {
  if (m_link.open()) {
    m_link.send(protocol::Goodbye{});
    m_link.close();
  }
}

Session::Event Session::update(f32 dt, const Controls& controls) {
  if (m_ended) return Event::None;

  Event      event  = Event::None;
  const bool closed = !m_link.open();

  while (auto message = m_link.poll()) {
    logging::net.trace("client <- {}", protocol::nameOf(*message));

    std::visit(overloaded{
                 [&](const protocol::Welcome& welcome) {
                   m_playerID   = welcome.playerID;
                   m_serverTick = welcome.tick;
                   event        = Event::Joined;
                   logging::client.info("Joined as player {} at tick {}", welcome.playerID, welcome.tick);
                 },
                 [&](const protocol::TickSync& sync) {
                   m_serverTick = sync.tick;
                 },
                 [&](const protocol::PlayerJoined& joined) {
                   m_players[joined.id] = joined.playerName;
                   logging::client.info("{} joined the game", joined.playerName);
                 },
                 [&](const protocol::PlayerLeft& left) {
                   if (auto itr = m_players.find(left.id); itr != m_players.end()) {
                     logging::client.info("{} left the game", itr->second);
                     m_players.erase(itr);
                   }
                 },
                 [&](const protocol::Disconnected& disconnected) {
                   end(disconnected.reason);
                 },
                 [&](const protocol::ChunkData& data) {
                   auto chunk = Chunk::decode(data.blocks);
                   if (!chunk) {
                     logging::net.warn("Malformed chunk ({}, {}), ignoring", data.position.x, data.position.y);
                     return;
                   }
                   m_world.insert(data.position, std::move(*chunk));
                 },
                 [&](const protocol::ChunkUnload& unload) {
                   m_world.erase(unload.position);
                 },
                 [&](const protocol::PlayerSync& sync) {
                   if (m_prediction)
                     m_prediction->reconcile(sync);
                   else
                     m_prediction.emplace(sync);
                 }
               },
      *message);

    if (m_ended) return Event::Ended;
  }

  if (m_prediction) {
    for (const protocol::PlayerInput& input : m_prediction->advance(dt, controls))
      m_link.send(input);
  }

  if (closed) {
    end("Connection lost");
    return Event::Ended;
  }

  return event;
}

bool Session::inGame() const {
  return m_playerID.has_value() && !m_ended;
}

bool Session::hosting() const {
  return m_game != nullptr;
}

std::optional<u16> Session::lanPort() const {
  return m_lan ? std::optional<u16>(m_lan->port()) : std::nullopt;
}

bool Session::openToLan(u16 port) {
  if (m_game == nullptr || m_lan != nullptr) return false;

  m_lan = net::Server::listen(net::Server::Info{.port = port, .joinChannel = m_game->joinChannel()});
  if (m_lan == nullptr) return false;

  logging::client.info("World opened to LAN on port {}", port);
  return true;
}

void Session::setViewDistance(i32 chunks) {
  if (m_viewDistance == chunks) return;
  m_viewDistance = chunks;
  m_link.send(protocol::ViewDistance{.chunks = static_cast<u32>(chunks)});
}

const ClientWorld& Session::world() const {
  return m_world;
}

std::vector<ChunkEvent> Session::takeChunkEvents() {
  return m_world.takeEvents();
}

const PlayerPrediction* Session::player() const {
  return m_prediction ? &*m_prediction : nullptr;
}

const str& Session::endReason() const {
  return m_endReason;
}

void Session::hello(std::string_view playerName) {
  m_link.send(protocol::Hello{.version = protocol::version, .playerName = str(playerName)});
}

void Session::end(str reason) {
  logging::client.info("Session ended: {}", reason);
  m_ended     = true;
  m_endReason = std::move(reason);
  m_link.close();
}
