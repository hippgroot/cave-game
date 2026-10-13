#include "client/session.hpp"

#include "core/log.hpp"
#include "core/overloaded.hpp"
#include "game/game.hpp"
#include "net/client.hpp"
#include "net/server.hpp"

std::unique_ptr<Session> Session::singleplayer(std::string_view playerName) {
  auto [client, server] = net::makeLinkPair();
  auto session          = std::unique_ptr<Session>(new Session(std::move(client)));

  session->m_game = std::make_unique<Game>();
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

Session::Event Session::update() {
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
                 }
               },
      *message);

    if (m_ended) return Event::Ended;
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
