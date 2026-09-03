add_executable(cave-game
  ${CMAKE_CURRENT_SOURCE_DIR}/app.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
)

target_compile_definitions(cave-game PRIVATE
  VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
  VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_link_libraries(cave-game PRIVATE engine-render game)
