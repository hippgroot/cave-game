add_library(game STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/game.cpp
)
add_library(CaveGame::game ALIAS game)

target_link_libraries(game PUBLIC CaveGame::core)
