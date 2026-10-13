#include "client/session.hpp"
#include "client/settings.hpp"
#include "client/ui/screen.hpp"
#include "client/ui/screens/main_menu.hpp"
#include "client/ui/screens/multiplayer_menu.hpp"
#include "client/ui/screens/pause_menu.hpp"
#include "client/ui/screens/settings_menu.hpp"
#include "client/ui/screens/singleplayer_menu.hpp"
#include "client/ui/screens/status_screen.hpp"
#include "core/log.hpp"
#include "core/threadbus.hpp"
#include "engine/renderer.hpp"

#include <grf/device.hpp>
#include <grf/gui.hpp>
#include <grf/imgui.h>
#include <grf/queue.hpp>
#include <grf/swapchain.hpp>
#include <grf/window.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

using namespace std::chrono_literals;

int main() {
  logging::configureFromEnvironment();
  logging::ThreadScope thread("main");

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

  auto openSettings = [&screens, &settings, &swapchain, &applySettings] {
    screens.push<SettingsMenu>(SettingsMenu::Info{
      .settings    = settings,
      .apply       = applySettings,
      .vsyncLocked = swapchain.vsyncOnly()
    });
  };

  std::unique_ptr<Session> session;
  MainMenu::Info           mainMenu;

  auto showStatus = [&screens](str title, str message, str button, std::function<void()> action) {
    screens.push<StatusScreen>(StatusScreen::Info{
      .title   = std::move(title),
      .message = std::move(message),
      .button  = std::move(button),
      .action  = std::move(action)
    });
  };

  auto endSession = [&screens, &session, &mainMenu, &showStatus](std::optional<str> reason) {
    session.reset();
    screens.clear();
    screens.push<MainMenu>(mainMenu);
    if (reason) showStatus("DISCONNECTED", *reason, "Back to Title", [&screens] { screens.pop(); });
  };

  auto startSession = [&session, &showStatus, &endSession](std::unique_ptr<Session> next, str title, str message) {
    session = std::move(next);
    showStatus(std::move(title), std::move(message), "Cancel", [&endSession] { endSession(std::nullopt); });
  };

  auto playWorld = [&session, &settings, &startSession](const WorldSave& world) {
    if (session != nullptr) return;
    logging::client.info("Opening world '{}' ({})", world.name, world.directory.string());
    startSession(Session::singleplayer(settings.general.playerName), "LOADING", world.name);
  };

  auto joinServer = [&session, &settings, &startSession, &showStatus, &screens](const ServerEntry& server) {
    if (session != nullptr) return;

    const auto address = net::Address::parse(server.address);
    if (!address) {
      showStatus("INVALID ADDRESS", server.address, "Back", [&screens] { screens.pop(); });
      return;
    }

    startSession(Session::join(*address, settings.general.playerName), "CONNECTING", address->toString());
  };

  auto openPauseMenu = [&screens, &session, &openSettings, &endSession] {
    screens.push<PauseMenu>(PauseMenu::Info{
      .actions = PauseMenu::Actions{
        .openToLan = [&screens, &session] {
          if (session != nullptr) session->openToLan();
          screens.pop(); },
        .settings  = openSettings,
        .quit      = [&endSession] { endSession(std::nullopt); }
      },
      .canOpenToLan = session->hosting() && !session->lanPort()
    });
  };

  mainMenu = MainMenu::Info{
    .actions = MainMenu::Actions{
      .singleplayer = [&screens, &playWorld] { screens.push<SingleplayerMenu>(SingleplayerMenu::Info{.play = playWorld}); },
      .multiplayer  = [&screens, &joinServer] { screens.push<MultiplayerMenu>(MultiplayerMenu::Info{.join = joinServer}); },
      .settings     = openSettings,
      .exit         = [&running] { running = false; }
    },
    .entrance = MainMenu::Entrance::Left
  };

  screens.push<MainMenu>(mainMenu);

  while (running && !window.shouldClose()) {
    window.pollEvents();

    auto dtopt = THREAD_BUS.wait_to_receive<f32>(beginFrameChannel, {}, 8ms);
    if (!dtopt) continue;
    f32 dt = *dtopt;
    profiler.logFrameTime(dt);

    if (session != nullptr) {
      switch (session->update()) {
      case Session::Event::Joined: screens.clear(); break;
      case Session::Event::Ended:  endSession(session->endReason()); break;
      case Session::Event::None:   break;
      }
    }

    const bool inGame = session != nullptr && session->inGame();

    if (inGame && window.isJustPressed(grf::Key::F3)) showDebug = !showDebug;

    gui.newFrame();

    const bool pausePressed = inGame && screens.empty() && ImGui::IsKeyPressed(ImGuiKey_Escape, false);

    screens.draw(dt);
    if (inGame && showDebug) profiler.draw();

    if (pausePressed) openPauseMenu();

    Renderer::FrameData frameData{
      .guiSnapshot   = gui.snapshot(),
      .cameraData    = std::nullopt,
      .addedMeshes   = {},
      .removedMeshes = {},
    };

    THREAD_BUS.send<Renderer::FrameData>(renderFrameChannel, std::move(frameData));
  }

  session.reset();
}
