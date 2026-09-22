#include "./app.hpp"

#include "engine/render.hpp"

#include "grf/device.hpp"
#include "grf/gui.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

using namespace grf;

i32 main() {
  logging::register_thread(logging::Thread::game);

  Window window(Window::Info{
    .width = 1280,
    .height = 720,
    .title = "Cave Game"
  });
  Device device;
  Swapchain swapchain(device, window);
  GUI gui(device, swapchain);
  Profiler profiler(device);

  Random rng;

  Channel<Renderer::FrameData>  newFrame;
  Channel<f32>                  beginFrame;
  Channel<Chunk>                chunkGenerated;
  Channel<Mesher::MeshResult>   meshGenerated;

  Renderer renderer(Renderer::Info{
    .device     = device,
    .swapchain  = swapchain,
    .gui        = gui,
    .profiler   = profiler,
    .newFrame   = newFrame,
    .beginFrame = beginFrame
  });

  JobPool jobPool;

  App app(App::Info{
    .window         = window,
    .gui            = gui,
    .profiler       = profiler,
    .rng            = rng,
    .chunkGenerated = chunkGenerated,
    .meshGenerated  = meshGenerated,
    .jobPool        = jobPool
  });

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
      .cameraData     = app.cameraData(),
      .addedMeshes    = app.addedMeshes(),
      .removedMeshes  = app.removedMeshes()
    });
  }
}