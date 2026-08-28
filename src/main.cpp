#include "core.hpp"
#include "render.hpp"

#include "grf/gui.hpp"
#include "grf/window.hpp"
#include "grf/swapchain.hpp"

int main() {
  logging::register_thread(logging::Thread::main);
  logging::general("Cave Game {}.{}.{}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

  Window window;
  Device device;
  Swapchain swapchain(device, window);
  GUI gui(device, swapchain);
  Profiler profiler(device);

  Channel<f32>        beginFrame;
  Channel<FrameData>  newFrame;

  Renderer renderer(Renderer::Info{
    .device     = device,
    .swapchain  = swapchain,
    .gui        = gui,
    .profiler   = profiler,
    .beginFrame = beginFrame,
    .newFrame   = newFrame
  });

  while (!window.shouldClose()) {
    auto dtopt = beginFrame.wait_to_receive({});
    if (!dtopt) break;
    f32 dt = *dtopt;

    window.pollEvents();
    profiler.logFrameTime(dt);

    gui.newFrame();
    profiler.draw();
    auto guiSnapshot = gui.snapshot();

    newFrame.send(FrameData{
      .guiSnapshot = std::move(guiSnapshot)
    });
  }
}