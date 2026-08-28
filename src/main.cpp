#include "core.hpp"

int main() {
  logging::register_thread(logging::Thread::main);
  logging::general("Cave Game {}.{}.{}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}