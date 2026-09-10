#include "./game.hpp"

Game::Game(Random& rng, JobPool& jobPool, Channel<Chunk>& chunkGenerated, World::Config config)
: m_rng(rng),
  m_jobPool(jobPool),
  m_world(rng, chunkGenerated, jobPool, std::move(config))
{}

void Game::update(const Input& input) {
  m_player.rotate(input.pitch, input.yaw);
  m_player.translate(input.worldUp, input.worldRight, input.dir);

  vec2 planePos = vec2(m_player.position().x, m_player.position().z);
  ivec2 center = ivec2(glm::floor((planePos - vec2(0.5f)) / static_cast<f32>(Chunk::size())));

  m_world.update(center, input.viewDistance);

  for (const auto& chunkCoord : m_world.addedChunks()) {
    auto& chunkopt = m_world[chunkCoord];
    if (chunkopt.has_value())
      m_events.emplace_back(ChunkLoadEvent{ .chunk = *chunkopt });
  }

  for (const auto& chunkCoord : m_world.removedChunks())
    m_events.emplace_back(ChunkUnloadEvent{ .coord = chunkCoord });
}

const Player& Game::player() const {
  return m_player;
}

std::vector<Game::Event> Game::events() {
  return std::exchange(m_events, {});
}