add_executable(cave-game
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/servers.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/servers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/worlds.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/worlds.cpp

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

  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/form_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/form_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/list_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/list_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/main_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/main_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/settings_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/multiplayer_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/multiplayer_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/pause_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/pause_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/settings_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/singleplayer_menu.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens/singleplayer_menu.cpp
)

target_link_libraries(cave-game PRIVATE
  CaveGame::engine
  CaveGame::game
  yaml-cpp::yaml-cpp
)
