#include "client/settings.hpp"
#include "client/ui/screen.hpp"
#include "client/ui/screens/main_menu.hpp"
#include "client/ui/screens/settings_menu.hpp"
#include "core/threadbus.hpp"
#include "engine/renderer.hpp"

#include <grf/device.hpp>
#include <grf/gui.hpp>
#include <grf/imgui.h>
#include <grf/queue.hpp>
#include <grf/swapchain.hpp>
#include <grf/window.hpp>

#include <filesystem>

using namespace std::chrono_literals;

int main() {
  const std::filesystem::path _assetsDir{ASSETS_DIR};

  ChannelID beginFrameChannel  = THREAD_BUS.registerChannel<f32>();
  ChannelID renderFrameChannel = THREAD_BUS.registerChannel<Renderer::FrameData>();

  Settings settings = Settings::load();

  grf::Window    window(grf::Window::Info{
    .width  = 1280,
    .height = 720,
    .title  = "Cave Game",
    .mode   = settings.video.windowMode
  });
  grf::Device    device;
  grf::Swapchain swapchain(device, window, grf::Swapchain::Info{.format = grf::Format::bgra8_srgb, .colorspace = grf::ColorSpace::srgb_nonlinear, .vsync = settings.video.vsync});
  grf::GUI       gui(device, swapchain);
  grf::Profiler  profiler(device);

  Renderer renderer(Renderer::Info{
    .device             = device,
    .swapchain          = swapchain,
    .gui                = gui,
    .profiler           = profiler,
    .beginFrameChannel  = beginFrameChannel,
    .renderFrameChannel = renderFrameChannel
  });

  ImGui::GetIO().IniFilename = nullptr;

  ScreenStack screens(ui::loadTheme(_assetsDir / "fonts" / "Monocraft-no-ligatures.ttc"));

  bool running   = true;
  bool showDebug = false;

  auto applySettings = [&window, &swapchain](const Settings& applied) {
    window.setMode(applied.video.windowMode);
    if (applied.video.vsync || swapchain.vsyncOnly())
      swapchain.enableVsync();
    else
      swapchain.disableVsync();
  };

  screens.push<MainMenu>(MainMenu::Info{
    .actions = MainMenu::Actions{
      .singleplayer = [] {},
      .multiplayer  = [] {},
      .settings     = [&screens, &settings, &swapchain, &applySettings] { screens.push<SettingsMenu>(SettingsMenu::Info{
                                                                            .settings    = settings,
                                                                            .apply       = applySettings,
                                                                            .vsyncLocked = swapchain.vsyncOnly()
                                                                          }); },
      .exit         = [&running] { running = false; }
    },
    .entrance = MainMenu::Entrance::Left
  });

  while (running && !window.shouldClose()) {
    window.pollEvents();

    auto dtopt = THREAD_BUS.wait_to_receive<f32>(beginFrameChannel, {}, 8ms);
    if (!dtopt) continue;
    f32 dt = *dtopt;
    profiler.logFrameTime(dt);

    if (window.isJustPressed(grf::Key::F3)) showDebug = !showDebug;

    gui.newFrame();
    screens.draw(dt);
    if (showDebug) profiler.draw();

    Renderer::FrameData frameData{
      .guiSnapshot   = gui.snapshot(),
      .cameraData    = std::nullopt,
      .addedMeshes   = {},
      .removedMeshes = {},
    };

    THREAD_BUS.send(renderFrameChannel, std::move(frameData));
  }
}
