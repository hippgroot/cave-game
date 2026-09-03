add_library(engine-render STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graveyard/graveyard.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graveyard/graveyard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer/renderer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer/renderer.cpp
)

target_compile_definitions(engine-render PUBLIC
  ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_include_directories(engine-render PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(engine-render PUBLIC grf engine-core)
