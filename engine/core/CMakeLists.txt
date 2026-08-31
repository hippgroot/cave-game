add_library(engine-core STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/channel.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/job_pool.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/types.hpp

  ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
)

target_include_directories(engine-core PRIVATE
  ${CMAKE_SOURCE_DIR}/external/grf/include
)
