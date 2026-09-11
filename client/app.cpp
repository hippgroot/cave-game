#include "./app.hpp"

#include "grf/imgui.h"
#include "grf/window.hpp"
#include "grf/gui.hpp"

#include <variant>

using namespace grf;

template <typename... Tps>
struct overloaded : Tps... { using Tps::operator()...; };

namespace {

constexpr ImGuiWindowFlags menuWindowFlags =
  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

void centerNextWindow() {
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

void configRow(const char* label) {
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::TableNextColumn();
  ImGui::SetNextItemWidth(-FLT_MIN);
}

}

App::App(const Info& info)
: m_window(info.window),
  m_gui(info.gui),
  m_profiler(info.profiler),
  m_rng(info.rng),
  m_chunkGenerated(info.chunkGenerated),
  m_meshGenerated(info.meshGenerated),
  m_jobPool(info.jobPool)
{
  setupGUIStyle();
}

void App::update(f32 dt) {
  Game::Input input = processInput(dt);
  drawGUI();

  if (!m_game) return;
  m_game->update(std::move(input));

  auto& player = m_game->player();
  m_camera->follow(player.position(), player.pitch(), player.yaw());

  for (auto& event : m_game->events()) {
    std::visit(overloaded{
      [&](Game::ChunkLoadEvent& e){
        if (m_idCache.contains(e.chunk.coord())) return;
        m_mesher->newMesh(std::move(e.chunk));
        m_idCache[e.chunk.coord()] = 0;
      },
      [&](Game::ChunkUnloadEvent& e){
        if (!m_idCache.contains(e.coord)) return;

        Renderer::MeshID meshID = m_idCache[e.coord];
        if (meshID == 0) return;

        m_removedMeshes.emplace_back(meshID);
        m_idCache.erase(e.coord);
      }
    }, event);
  }

  for (auto& [coord, meshData] : m_mesher->receiveMeshes()) {
    if (!m_idCache.contains(coord)) continue;
    m_idCache[coord] = meshData.meshID;
    m_addedMeshes.emplace_back(std::move(meshData));
  }
}

bool App::exitRequested() const {
  return m_exitRequested;
}

std::optional<Camera::UpdateData> App::cameraData() const {
  return m_camera ? m_camera->update() : std::nullopt;
}

std::vector<MeshData> App::addedMeshes() {
  return std::exchange(m_addedMeshes, {});
}

std::vector<Renderer::MeshID> App::removedMeshes() {
  return std::exchange(m_removedMeshes, {});
}

void App::setupGUIStyle() const {
  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.IniFilename = nullptr;

  io.Fonts->AddFontFromFileTTF(ASSETS_DIR "/fonts/Monocraft-no-ligatures.ttc", 24.0f);

  auto& style = ImGui::GetStyle();

  style.WindowRounding    = 0.0f;
  style.ChildRounding     = 0.0f;
  style.FrameRounding     = 0.0f;
  style.PopupRounding     = 0.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabRounding      = 0.0f;
  style.TabRounding       = 0.0f;

  style.WindowBorderSize  = 0.0f;
  style.FrameBorderSize   = 1.0f;
  style.PopupBorderSize   = 1.0f;

  style.WindowPadding     = ImVec2(16.0f, 16.0f);
  style.FramePadding      = ImVec2(12.0f, 10.0f);
  style.ItemSpacing       = ImVec2(8.0f, 8.0f);
  style.GrabMinSize       = 16.0f;
  style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
  style.ButtonTextAlign   = ImVec2(0.5f, 0.5f);

  auto& colors = style.Colors;

  colors[ImGuiCol_Text]           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled]   = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
  colors[ImGuiCol_Border]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_BorderShadow]   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  colors[ImGuiCol_WindowBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);
  colors[ImGuiCol_ChildBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg]        = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);

  colors[ImGuiCol_Button]         = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
  colors[ImGuiCol_ButtonHovered]  = ImVec4(0.51f, 0.56f, 0.79f, 1.00f);
  colors[ImGuiCol_ButtonActive]   = ImVec4(0.36f, 0.40f, 0.60f, 1.00f);

  colors[ImGuiCol_FrameBg]        = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
  colors[ImGuiCol_FrameBgActive]  = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

  colors[ImGuiCol_SliderGrab]     = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.51f, 0.56f, 0.79f, 1.00f);
  colors[ImGuiCol_CheckMark]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

  colors[ImGuiCol_TitleBg]        = ImVec4(0.16f, 0.11f, 0.08f, 1.00f);
  colors[ImGuiCol_TitleBgActive]  = ImVec4(0.24f, 0.16f, 0.11f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.11f, 0.08f, 0.80f);

  colors[ImGuiCol_Header]         = ImVec4(0.48f, 0.48f, 0.48f, 0.60f);
  colors[ImGuiCol_HeaderHovered]  = ImVec4(0.51f, 0.56f, 0.79f, 0.80f);
  colors[ImGuiCol_HeaderActive]   = ImVec4(0.36f, 0.40f, 0.60f, 1.00f);

  colors[ImGuiCol_ScrollbarBg]    = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
  colors[ImGuiCol_ScrollbarGrab]  = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);

  colors[ImGuiCol_Tab]            = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
  colors[ImGuiCol_TabHovered]     = ImVec4(0.51f, 0.56f, 0.79f, 1.00f);
  colors[ImGuiCol_TabSelected]    = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);

  colors[ImGuiCol_PlotLines]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_PlotHistogram]  = ImVec4(0.51f, 0.56f, 0.79f, 1.00f);

  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.51f, 0.56f, 0.79f, 1.00f);
  colors[ImGuiCol_SeparatorActive]  = ImVec4(0.36f, 0.40f, 0.60f, 1.00f);
}

Game::Input App::processInput(f32 dt) {
  Game::Input input{
    .worldUp      = Camera::worldUp(),
    .worldRight   = Camera::worldRight(),
    .dir          = vec3(0.0f),
    .pitch        = 0.0f,
    .yaw          = 0.0f,
    .viewDistance = m_settings.viewDistance
  };

  if (m_window.isJustPressed(Key::F3))
    m_debugEnabled = !m_debugEnabled;

  if (m_window.isJustPressed(Key::Escape)) {
    switch (m_state) {
    using enum UIState;
    case Closed:
      m_state = PauseMenu;
      break;
    case MainMenu:
      break;
    case GenerateMenu:
      m_state = MainMenu;
      break;
    case SettingsMenu:
      m_state = m_game ? PauseMenu : MainMenu;
      break;
    case PauseMenu:
      m_state = Closed;
    }
  }

  if (m_state == UIState::Closed && !m_gui.ownsInput()) {
    vec3 dir = vec3(
      static_cast<float>(m_window.isPressed(Key::D) - m_window.isPressed(Key::A)),
      static_cast<float>(m_window.isPressed(Key::Space) - m_window.isPressed(Key::LeftShift)),
      static_cast<float>(m_window.isPressed(Key::S) - m_window.isPressed(Key::W))
    );
    input.dir = glm::dot(dir, dir) < 1e-4f ? vec3(0.0f) : glm::normalize(dir) * dt;

    if (m_window.isPressed(MouseButton::Left)) {
      vec2 cursorDelta = m_window.cursorDelta();
      input.pitch = -cursorDelta.y * m_settings.sensitivity;
      input.yaw = -cursorDelta.x * m_settings.sensitivity;
    }
  }

  return input;
}

void App::drawGUI() {
  if (m_debugEnabled)
    gui_debugOverlay();

  switch (m_state) {
  using enum UIState;
  case Closed:
    return;
  case MainMenu:
    gui_mainMenu();
    break;
  case GenerateMenu:
    gui_generateMenu();
    break;
  case SettingsMenu:
    gui_settingsMenu();
    break;
  case PauseMenu:
    gui_pauseMenu();
  }
}

void App::gui_mainMenu() {
  centerNextWindow();
  ImGui::Begin("Main Menu", nullptr, menuWindowFlags | ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("New World"))
      m_state = UIState::GenerateMenu;
    if (ImGui::Button("Settings"))
      m_state = UIState::SettingsMenu;
    if (ImGui::Button("Exit"))
      m_exitRequested = true;
  ImGui::End();
}

void App::gui_generateMenu() {
  static World::Config _worldConfig{
    .seed               = (static_cast<i64>(m_rng.i(0)) << 32) | static_cast<i64>(m_rng.i(0)),
    .surfaceFrequency   = m_rng.f(0.001f, 0.1f),
    .surfaceLacunarity  = m_rng.f(1.0f, 5.0f),
    .surfaceGain        = m_rng.f(0.01f, 0.99f),
    .surfaceOctaves     = m_rng.i(3, 10),
    .caveFrequency      = m_rng.f(0.001f, 0.1f),
    .caveThreshold      = m_rng.f(0.2f, 0.8f)
  };

  centerNextWindow();
  ImGui::SetNextWindowSize(ImVec2(560.0f, 460.0f));
  ImGui::Begin("Generate World", nullptr, menuWindowFlags);
    ImVec2 tableSize(0.0f, -ImGui::GetFrameHeightWithSpacing());
    if (ImGui::BeginTable("##worldconfig", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg, tableSize)) {
      ImGui::TableSetupColumn("##item", ImGuiTableColumnFlags_WidthFixed, 220.0f);
      ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

      configRow("Seed");
      ImGui::InputScalar("##seed", ImGuiDataType_U64, &_worldConfig.seed);

      configRow("Surface Frequency");
      ImGui::SliderFloat("##surfacefrequency", &_worldConfig.surfaceFrequency, 0.001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);

      configRow("Surface Lacunarity");
      ImGui::SliderFloat("##surfacelacunarity", &_worldConfig.surfaceLacunarity, 1.0f, 5.0f, "%.2f");

      configRow("Surface Gain");
      ImGui::SliderFloat("##surfacegain", &_worldConfig.surfaceGain, 0.01f, 0.99f, "%.2f");

      configRow("Surface Octaves");
      ImGui::SliderInt("##surfaceoctaves", &_worldConfig.surfaceOctaves, 3, 10);

      configRow("Cave Frequency");
      ImGui::SliderFloat("##cavefrequency", &_worldConfig.caveFrequency, 0.001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);

      configRow("Cave Threshold");
      ImGui::SliderFloat("##cavethreshold", &_worldConfig.caveThreshold, 0.0f, 1.0f, "%.2f");

      ImGui::EndTable();
    }

    if (ImGui::Button("Play")) {
      m_game    = std::make_unique<Game>(m_rng, m_jobPool, m_chunkGenerated, _worldConfig);
      m_mesher  = std::make_unique<Mesher>(m_meshGenerated, m_jobPool);
      m_camera  = std::make_unique<Camera>();

      auto& player = m_game->player();
      m_camera->setFOV(m_settings.fov_deg);
      m_camera->setViewport(m_window.width(), m_window.height());
      m_camera->follow(player.position(), player.pitch(), player.yaw());

      m_state = UIState::Closed;
    }

    if (ImGui::Button("Back")) {
      m_state = UIState::MainMenu;
      _worldConfig = World::Config{
        .seed               = (static_cast<i64>(m_rng.i(0)) << 32) | static_cast<i64>(m_rng.i(0)),
        .surfaceFrequency   = m_rng.f(0.001f, 0.1f),
        .surfaceLacunarity  = m_rng.f(1.0f, 5.0f),
        .surfaceGain        = m_rng.f(0.01f, 0.99f),
        .surfaceOctaves     = m_rng.i(3, 10),
        .caveFrequency      = m_rng.f(0.001f, 0.1f),
        .caveThreshold      = m_rng.f(0.2f, 0.8f)
      };
    }
  ImGui::End();
}

void App::gui_settingsMenu() {
  centerNextWindow();
  ImGui::SetNextWindowSize(ImVec2(560.0f, 400.0f));
  ImGui::Begin("Settings", nullptr, menuWindowFlags);
    ImVec2 tableSize(0.0f, -ImGui::GetFrameHeightWithSpacing());
    if (ImGui::BeginTable("##settings", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg, tableSize)) {
      ImGui::TableSetupColumn("##item", ImGuiTableColumnFlags_WidthFixed, 220.0f);
      ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Sensitivity");
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::SliderFloat("##sensitivity", &m_settings.sensitivity, 0.0001f, 0.01f, "%.4f");

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("FOV");
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::SliderFloat("##fov", &m_settings.fov_deg, 60.0f, 110.0f, "%.0f");

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("View Distance");
      ImGui::TableNextColumn();
      ImGui::SetNextItemWidth(-FLT_MIN);
      ImGui::SliderInt("##viewdistance", &m_settings.viewDistance, 2, 32);

      ImGui::EndTable();
    }
    if (ImGui::Button("Back"))
      m_state = m_game ? UIState::PauseMenu : UIState::MainMenu;
  ImGui::End();
}

void App::gui_pauseMenu() {
  centerNextWindow();
  ImGui::Begin("Paused", nullptr, menuWindowFlags);
    if (ImGui::Button("Resume"))
      m_state = UIState::Closed;
    if (ImGui::Button("Settings"))
      m_state = UIState::SettingsMenu;
    if (ImGui::Button("Exit to Main Menu")) {
      m_state = UIState::MainMenu;
      m_camera.reset();
      m_mesher.reset();
      m_game.reset();
    }
    if (ImGui::Button("Exit to Desktop"))
      m_exitRequested = true;
  ImGui::End();
}

void App::gui_debugOverlay() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 corner(
    viewport->WorkPos.x + viewport->WorkSize.x - 16.0f,
    viewport->WorkPos.y + viewport->WorkSize.y - 16.0f
  );
  ImGui::SetNextWindowPos(corner, ImGuiCond_FirstUseEver, ImVec2(1.0f, 1.0f));
  ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
  m_profiler.draw();
}