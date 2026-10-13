#include "client/settings.hpp"
#include "client/ui/screen.hpp"
#include "client/ui/screens/main_menu.hpp"
#include "client/ui/screens/multiplayer_menu.hpp"
#include "client/ui/screens/pause_menu.hpp"
#include "client/ui/screens/settings_menu.hpp"
#include "client/ui/screens/singleplayer_menu.hpp"
#include "core/network.hpp"
#include "core/overloaded.hpp"
#include "core/threadbus.hpp"
#include "engine/renderer.hpp"
#include "game/game.hpp"

#include <grf/device.hpp>
#include <grf/gui.hpp>
#include <grf/imgui.h>
#include <grf/queue.hpp>
#include <grf/swapchain.hpp>
#include <grf/window.hpp>

#include <filesystem>
#include <print>

using namespace std::chrono_literals;

int main() {
  const std::filesystem::path _assetsDir{ASSETS_DIR};

  ChannelID beginFrameChannel = THREAD_BUS.registerChannel<f32>();
  ChannelID renderFrameChannel =
    THREAD_BUS.registerChannel<Renderer::FrameData>();

  Settings settings = Settings::load();

  grf::Window    window(grf::Window::Info{
    .width = 1280, .height = 720, .title = "Cave Game", .mode = settings.video.windowMode
  });
  grf::Device    device;
  grf::Swapchain swapchain(
    device, window,
    grf::Swapchain::Info{
      .format = grf::Format::bgra8_srgb, .colorspace = grf::ColorSpace::srgb_nonlinear, .vsync = settings.video.vsync
    });
  grf::GUI      gui(device, swapchain);
  grf::Profiler profiler(device);

  Renderer renderer(Renderer::Info{
    .device = device, .swapchain = swapchain, .gui = gui, .profiler = profiler, .beginFrameChannel = beginFrameChannel, .renderFrameChannel = renderFrameChannel
  });

  ImGui::GetIO().IniFilename = nullptr;

  ScreenStack screens(
    ui::loadTheme(_assetsDir / "fonts" / "Monocraft-no-ligatures.ttc"));

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
    screens.push<SettingsMenu>(
      SettingsMenu::Info{.settings = settings, .apply = applySettings, .vsyncLocked = swapchain.vsyncOnly()});
  };

  std::unique_ptr<ConnectionStrategy> connection = nullptr;
  std::unique_ptr<Game>               game       = nullptr;
  std::optional<u64>                  playerID   = std::nullopt;

  MainMenu::Info mainMenu;

  auto leaveWorld = [&screens, &game, &connection, &playerID, &mainMenu] {
    if (connection != nullptr)
      connection->send(protocol::Goodbye{});

    game.reset();
    connection.reset();
    playerID.reset();

    screens.clear();
    screens.push<MainMenu>(mainMenu);
  };

  auto playWorld = [&game, &connection](const WorldSave& world) {
    if (connection != nullptr)
      return;

    std::println("Play world '{}' ({})", world.name, world.directory.string());
    game = std::make_unique<Game>();

    auto conn = std::make_unique<LocalConnection>();

    auto peer = std::make_unique<LocalPeer>();
    peer->linkListenChannel(conn->peerListenChannel());
    peer->linkSendChannel(conn->peerSendChannel());

    THREAD_BUS.send<std::unique_ptr<PeerStrategy>>(game->joinChannel(), std::move(peer));

    conn->send(protocol::Hello{.version = protocol::currentVersion, .playerName = "Player"});
    connection = std::move(conn);
  };

  mainMenu = MainMenu::Info{
    .actions = MainMenu::Actions{
      .singleplayer =
        [&screens, &playWorld] {
          screens.push<SingleplayerMenu>(SingleplayerMenu::Info{.play = playWorld});
        },
      .multiplayer =
        [&screens] {
          screens.push<MultiplayerMenu>(MultiplayerMenu::Info{
            .join = [](const ServerEntry& server) {
              std::println("Join server '{}' at {}", server.name,
                server.address);
            }
          });
        },
      .settings = openSettings,
      .exit     = [&running] { running = false; }
    },
    .entrance = MainMenu::Entrance::Left
  };

  screens.push<MainMenu>(mainMenu);

  while (running && !window.shouldClose()) {
    window.pollEvents();

    auto dtopt = THREAD_BUS.wait_to_receive<f32>(beginFrameChannel, {}, 8ms);
    if (!dtopt)
      continue;
    f32 dt = *dtopt;
    profiler.logFrameTime(dt);

    if (connection != nullptr) {
      using namespace protocol;
      while (auto msgopt = connection->poll()) {
        std::visit(overloaded{
                     [&](const Welcome& welcome) {
                       playerID = welcome.playerID;
                       screens.clear();
                     },
                     [&](const TickSync& tickSync) {},
                     [&](const PlayerJoined& playerJoined) {},
                     [&](const PlayerLeft& playerLeft) {},
                     [&](const Disconnected& disconnected) {
                       std::println("Disconnected: {}", disconnected.reason);
                       leaveWorld();
                     }
                   },
          std::move(*msgopt));
        if (connection == nullptr)
          break;
      }
    }

    const bool inGame = playerID.has_value();

    if (inGame && window.isJustPressed(grf::Key::F3))
      showDebug = !showDebug;

    gui.newFrame();

    const bool openPause = inGame && screens.empty() && ImGui::IsKeyPressed(ImGuiKey_Escape, false);

    screens.draw(dt);
    if (inGame && showDebug)
      profiler.draw();

    if (openPause) {
      screens.push<PauseMenu>(PauseMenu::Info{
        .actions = PauseMenu::Actions{
          .openToLan = [] { std::println("Open to LAN"); },
          .settings  = openSettings,
          .quit      = leaveWorld
        },
        .canOpenToLan = game != nullptr
      });
    }

    Renderer::FrameData frameData{
      .guiSnapshot   = gui.snapshot(),
      .cameraData    = std::nullopt,
      .addedMeshes   = {},
      .removedMeshes = {},
    };

    THREAD_BUS.send(renderFrameChannel, std::move(frameData));
  }
}
