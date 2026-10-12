cmake_minimum_required(VERSION 4.2)

project(CaveGame
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)

set(GRF_BUILD_EXAMPLES OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/grf)

add_library(FastNoiseLite INTERFACE)
target_include_directories(FastNoiseLite SYSTEM INTERFACE
${PROJECT_SOURCE_DIR}/external/FastNoiseLite/Cpp
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/core)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/client)
