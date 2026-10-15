add_library(game STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/game.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/game.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/player/body.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/controls.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/controls.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/player.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/player.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/spectator.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/spectator.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/world/biome.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/biome.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/block.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/chunk.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/chunk.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/world.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/world.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/biome_source.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/biome_source.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/climate.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/climate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/pipeline.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/rng.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/shape.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/shape.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stage.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/biomes.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/biomes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/carvers.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/carvers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/features.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/features.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/finalize.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/finalize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/surface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/terrain.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/world/gen/stages/terrain.cpp
)
add_library(CaveGame::game ALIAS game)

target_link_libraries(game
  PUBLIC
    CaveGame::core
    CaveGame::net
  PRIVATE
    FastNoiseLite
)
