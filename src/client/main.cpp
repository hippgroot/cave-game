#include <grf/device.hpp>
#include <grf/gui.hpp>
#include <grf/swapchain.hpp>
#include <grf/window.hpp>

using namespace grf;

int main() {
  Window    window(Window::Info{
    .width  = 1280,
    .height = 720,
    .title  = "Cave Game"
  });
  Device    device;
  Swapchain swapchain(device, window);
  GUI       gui(device, swapchain);
  Profiler  profiler(device);

  while (!window.shouldClose()) {
    window.pollEvents();
  }

  device.waitIdle();
}
