#include "./app.hpp"

#include "grf/imgui.h"
#include "grf/window.hpp"
#include "grf/gui.hpp"

using namespace grf;

namespace {

constexpr ImGuiWindowFlags menuWindowFlags =
  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

void centerNextWindow() {
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
}

}

App::App(const Window& window, const GUI& gui, const Profiler& profiler)
: m_window(window),
  m_gui(gui),
  m_profiler(profiler)
{
  setupGUIStyle();
}

void App::update(f32 dt) {
  processInput();
  drawGUI();
}

bool App::exitRequested() const {
  return m_exitRequested;
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

void App::processInput() {
  if (m_gui.ownsInput()) return;

  if (m_window.isJustPressed(Key::Escape))
    m_debugEnabled = !m_debugEnabled;
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
  centerNextWindow();
  ImGui::Begin("Generate World", nullptr, menuWindowFlags | ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("Back"))
      m_state = UIState::MainMenu;
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
      m_state = UIState::MainMenu;
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