#pragma once

#include "core/types.hpp"

#include <grf/resources.hpp>

#include <utility>
#include <variant>
#include <vector>

class Graveyard {
  using ValidType = std::variant<grf::Buffer, grf::Image>;
  std::vector<std::pair<u32, ValidType>> m_retired;

public:
  Graveyard()  = default;
  ~Graveyard() = default;

  Graveyard(const Graveyard&)            = delete;
  Graveyard(Graveyard&&)                 = delete;
  Graveyard& operator=(const Graveyard&) = delete;
  Graveyard& operator=(Graveyard&&)      = delete;

  void retire(ValidType resource, u32 frameDelay);
  void update();
};
