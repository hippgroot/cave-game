add_library(game STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/game.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/game.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/player.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/player/player.cpp
  # ${CMAKE_CURRENT_SOURCE_DIR}/world/block.hpp
  # ${CMAKE_CURRENT_SOURCE_DIR}/world/chunk.hpp
  # ${CMAKE_CURRENT_SOURCE_DIR}/world/world.hpp
  # ${CMAKE_CURRENT_SOURCE_DIR}/world/world.cpp
)

target_include_directories(game PUBLIC ${CMAKE_SOURCE_DIR})

target_include_directories(game PRIVATE
  ${CMAKE_SOURCE_DIR}/external/grf/include
)

target_link_libraries(game PUBLIC engine-core)
