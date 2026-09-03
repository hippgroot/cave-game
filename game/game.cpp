#include "./game.hpp"

Game::Game(WorldConfig config) {}

void Game::update(const Input& input) {
  m_player.rotate(input.pitch, input.yaw);
  m_player.translate(input.worldUp, input.worldRight, input.dir);
}

const Player& Game::player() const {
  return m_player;
}