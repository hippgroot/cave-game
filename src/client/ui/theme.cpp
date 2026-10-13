#include "client/ui/theme.hpp"

#include <grf/imgui.h>

namespace ui {

namespace {

constexpr int _regularFace = 0;
constexpr int _boldFace    = 4;

ImFont* loadFace(const std::filesystem::path& path, int face) {
  ImFontConfig config;
  config.FontNo = face;
  return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), 0.0f, &config);
}

}

Theme loadTheme(const std::filesystem::path& fontPath) {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  if (io.Fonts->Fonts.empty()) io.Fonts->AddFontDefault();

  Theme theme;
  theme.bodyFont  = loadFace(fontPath, _regularFace);
  theme.titleFont = loadFace(fontPath, _boldFace);
  return theme;
}

}
