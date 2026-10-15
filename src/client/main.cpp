#include "client/controller.hpp"
#include "client/mesher.hpp"
#include "client/session.hpp"
#include "client/settings.hpp"
#include "client/ui/debug_overlay.hpp"
#include "client/ui/screen.hpp"
#include "client/ui/screens/main_menu.hpp"
#include "client/ui/screens/multiplayer_menu.hpp"
#include "client/ui/screens/pause_menu.hpp"
#include "client/ui/screens/settings_menu.hpp"
#include "client/ui/screens/singleplayer_menu.hpp"
#include "client/ui/screens/status_screen.hpp"
#include "core/log.hpp"
#include "core/threadbus.hpp"
#include "engine/camera.hpp"
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

  Controller controller;
  Mesher     mesher;
  Camera     camera;
  uvec2      viewport = uvec2(0);
  f32        fov      = 0.0f;

  std::unique_ptr<Session> session;

  auto applySettings = [&window, &swapchain, &session](const Settings& applied) {
    window.setMode(applied.video.windowMode);
    if (applied.video.vsync || swapchain.vsyncOnly())
      swapchain.enableVsync();
    else
      swapchain.disableVsync();
    if (session != nullptr) session->setViewDistance(applied.video.viewDistance);
  };

  auto openSettings = [&screens, &settings, &swapchain, &applySettings] {
    screens.push<SettingsMenu>(SettingsMenu::Info{
      .settings    = settings,
      .apply       = applySettings,
      .vsyncLocked = swapchain.vsyncOnly()
    });
  };

  MainMenu::Info mainMenu;

  auto showStatus = [&screens](str title, str message, str button, std::function<void()> action) {
    screens.push<StatusScreen>(StatusScreen::Info{
      .title   = std::move(title),
      .message = std::move(message),
      .button  = std::move(button),
      .action  = std::move(action)
    });
  };

  auto endSession = [&screens, &session, &mesher, &mainMenu, &showStatus](std::optional<str> reason) {
    session.reset();
    mesher.clear();
    screens.clear();
    screens.push<MainMenu>(mainMenu);
    if (reason) showStatus("DISCONNECTED", *reason, "Back to Title", [&screens] { screens.pop(); });
  };

  auto startSession = [&session, &settings, &controller, &showStatus, &endSession](std::unique_ptr<Session> next, str title, str message) {
    session = std::move(next);
    controller.reset();
    session->setViewDistance(settings.video.viewDistance);
    showStatus(std::move(title), std::move(message), "Cancel", [&endSession] { endSession(std::nullopt); });
  };

  auto playWorld = [&session, &settings, &startSession](const WorldSave& world) {
    if (session != nullptr) return;
    logging::client.info("Opening world '{}' ({})", world.name, world.directory.string());
    startSession(Session::singleplayer(world.seed, settings.general.playerName), "LOADING", world.name);
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

    const bool playing = session != nullptr && session->inGame() && screens.empty();
    window.setCursorCaptured(playing);
    if (playing)
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else
      ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    const Controls controls = controller.sample(window, settings.controls, playing);

    if (session != nullptr) {
      switch (session->update(dt, controls)) {
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

    const Session* active = session != nullptr && session->inGame() ? session.get() : nullptr;

    if (active != nullptr && showDebug) {
      profiler.draw();
      ui::debugOverlay(*active, mesher);
    }

    std::optional<Camera::UpdateData> cameraData;
    if (active != nullptr && active->player() != nullptr) {
      const PlayerPrediction& player = *active->player();

      if (const uvec2 size = uvec2(window.width(), window.height()); size != viewport) {
        viewport = size;
        camera.setViewport(size.x, size.y);
      }
      if (settings.video.fov != fov) {
        fov = settings.video.fov;
        camera.setFOV(fov);
      }

      camera.follow(player.eye(), player.pitch(), player.yaw());
      cameraData = camera.update();
    }

    if (session != nullptr) {
      const ChunkPos center = session->player() != nullptr ? Chunk::posOf(ivec3(glm::floor(session->player()->position()))) : ChunkPos(0);
      mesher.update(session->takeChunkEvents(), session->world(), center);
    }
    Mesher::Changes meshes = mesher.take();

    if (pausePressed) openPauseMenu();

    Renderer::FrameData frameData{
      .guiSnapshot   = gui.snapshot(),
      .cameraData    = std::move(cameraData),
      .addedMeshes   = std::move(meshes.added),
      .removedMeshes = std::move(meshes.removed),
    };

    THREAD_BUS.send<Renderer::FrameData>(renderFrameChannel, std::move(frameData));
  }

  session.reset();
}
