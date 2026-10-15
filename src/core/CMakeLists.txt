find_package(Threads REQUIRED)

add_library(core STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/files.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/overloaded.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/paths.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/threadbus.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/types.hpp
)
add_library(CaveGame::core ALIAS core)

target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_features(core PUBLIC cxx_std_26)
target_link_libraries(core PUBLIC Threads::Threads grf)
