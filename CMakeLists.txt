cmake_minimum_required(VERSION 4.2)

project(CaveGame
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)

set(GRF_BUILD_EXAMPLES OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/grf)

add_subdirectory(${CMAKE_SOURCE_DIR}/engine/core)
add_subdirectory(${CMAKE_SOURCE_DIR}/engine/render)
add_subdirectory(${CMAKE_SOURCE_DIR}/game)

add_subdirectory(${CMAKE_SOURCE_DIR}/client)
