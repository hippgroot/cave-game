#include "engine/graveyard.hpp"

void Graveyard::retire(ValidType resource, u32 frameDelay) {
  m_retired.emplace_back(frameDelay, std::move(resource));
}

void Graveyard::update() {
  for (auto& [frames, resource] : m_retired)
    --frames;
  std::erase_if(m_retired, [](const auto& r) { return r.first == 0; });
}
