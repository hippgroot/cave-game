add_library(engine STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/assets.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/assets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/camera.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/camera.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graveyard.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graveyard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp
)
add_library(CaveGame::engine ALIAS engine)

target_compile_definitions(engine PUBLIC
  ASSETS_DIR="${PROJECT_SOURCE_DIR}/assets"
)

target_compile_features(engine PUBLIC cxx_std_26)

target_link_libraries(engine
  PUBLIC
    CaveGame::core
    grf
  PRIVATE
    yaml-cpp::yaml-cpp
)
