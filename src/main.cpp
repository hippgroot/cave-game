#include "core/types.hpp"
#include "renderer.hpp"
#include "random.hpp"
#include "world/world.hpp"
#include "game.hpp"

#include "grf/gui.hpp"
#include "grf/window.hpp"
#include "grf/swapchain.hpp"

using namespace grf;

int main() {
  logging::register_thread(logging::Thread::game);
  logging::general("Cave Game {}.{}.{}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

  Window window;
  Device device;
  Swapchain swapchain(device, window);
  GUI gui(device, swapchain);
  Profiler profiler(device);

  Random rand;

  Channel<f32>                  beginFrame;
  Channel<Renderer::FrameData>  newFrame;
  Channel<World::GenRes>        chunkGenerated;

  JobPool jobPool;

  Renderer renderer(Renderer::Info{
    .device     = device,
    .swapchain  = swapchain,
    .gui        = gui,
    .profiler   = profiler,
    .newFrame   = newFrame,
    .beginFrame = beginFrame
  });

  Game game(Game::Info{
    .window         = window,
    .gui            = gui,
    .profiler       = profiler,
    .rand           = rand,
    .chunkGenerated = chunkGenerated,
    .jobPool        = jobPool
  });

  while (!window.shouldClose()) {
    auto dtopt = beginFrame.block_and_receive({});
    if (!dtopt) break;
    f32 dt = *dtopt;

    window.pollEvents();
    profiler.logFrameTime(dt);
    game.update(dt);

    newFrame.send(game.snapshot());
  }
}