#include "./graveyard.hpp"

#include "grf/resources.hpp"

void Graveyard::retire(ValidType resource, u32 frameDelay) {
  m_retired.emplace_back(frameDelay, resource);
}

void Graveyard::update() {
  for (auto& [f, r] : m_retired) --f;
  std::erase_if(m_retired, [](auto r){ return r.first == 0; });
}