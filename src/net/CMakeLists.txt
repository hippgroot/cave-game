add_library(net STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/address.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/codec.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/link.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/protocol.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/enet.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/enet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/client.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)
add_library(CaveGame::net ALIAS net)

target_link_libraries(net
  PUBLIC
    CaveGame::core
  PRIVATE
    enet
)
