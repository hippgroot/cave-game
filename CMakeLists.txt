cmake_minimum_required(VERSION 4.2)

project(CaveGame
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)

set(GRF_BUILD_EXAMPLES OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/grf)

add_executable(cave-game
  ${CMAKE_SOURCE_DIR}/game/world/world.hpp
  ${CMAKE_SOURCE_DIR}/game/world/world.cpp
  ${CMAKE_SOURCE_DIR}/game/camera/camera.hpp
  ${CMAKE_SOURCE_DIR}/game/camera/camera.cpp

  ${CMAKE_SOURCE_DIR}/include/core.hpp
  ${CMAKE_SOURCE_DIR}/include/game.hpp
  ${CMAKE_SOURCE_DIR}/include/render.hpp

  ${CMAKE_SOURCE_DIR}/src/game.cpp
  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/render.cpp
)

target_compile_definitions(cave-game PRIVATE
  VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
  VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_include_directories(cave-game PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/game
  ${CMAKE_SOURCE_DIR}/external/FastNoiseLite/Cpp
)

target_link_libraries(cave-game PRIVATE grf)
