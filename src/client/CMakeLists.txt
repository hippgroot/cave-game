add_executable(cave-game
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/ui/anim.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/anim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/context.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/layout.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/rect.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screen.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/theme.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/theme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/widgets.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/widgets.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/main_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/main_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/settings_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/settings_menu.cpp
)

target_include_directories(cave-game PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(cave-game PRIVATE
  CaveGame::engine
  yaml-cpp::yaml-cpp
)
