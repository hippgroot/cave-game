#include "./app.hpp"

#include "engine/render.hpp"

#include "grf/device.hpp"
#include "grf/gui.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

using namespace grf;

i32 main() {
  logging::register_thread(logging::Thread::game);

  Window window;
  Device device;
  Swapchain swapchain(device, window);
  GUI gui(device, swapchain);
  Profiler profiler(device);

  Channel<Renderer::FrameData>  newFrame;
  Channel<f32>                  beginFrame;

  Renderer renderer(Renderer::Info{
    .device     = device,
    .swapchain  = swapchain,
    .gui        = gui,
    .profiler   = profiler,
    .newFrame   = newFrame,
    .beginFrame = beginFrame
  });

  JobPool jobPool;

  App app(window, gui, profiler);

  while (!window.shouldClose() && !app.exitRequested()) {
    auto dtopt = beginFrame.block_and_receive({});
    if (!dtopt) break;
    f32 dt = *dtopt;

    window.pollEvents();
    profiler.logFrameTime(dt);

    gui.newFrame();
    app.update(dt);

    newFrame.send(Renderer::FrameData{
      .guiSnapshot    = gui.snapshot(),
      .cameraData     = std::nullopt,
      .addedMeshes    = {},
      .removedMeshes  = {}
    });
  }
}