find_package(Threads REQUIRED)

add_library(core INTERFACE)
add_library(CaveGame::core ALIAS core)

target_include_directories(core INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_compile_features(core INTERFACE cxx_std_26)
target_link_libraries(core INTERFACE Threads::Threads grf)
