add_executable(cave-game
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_include_directories(cave-game PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(cave-game PRIVATE grf)
