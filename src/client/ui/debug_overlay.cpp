#include "client/ui/debug_overlay.hpp"

#include "client/mesher.hpp"
#include "client/session.hpp"
#include "game/world/chunk.hpp"

#include <grf/imgui.h>

#include <cmath>

namespace ui {

namespace {

constexpr f32 _margin     = 8.0f;
constexpr f32 _background = 0.5f;

constexpr ImGuiWindowFlags _flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

}

void debugOverlay(const Session& session, const Mesher& mesher) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - _margin, viewport->WorkPos.y + _margin), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowBgAlpha(_background);

  if (ImGui::Begin("##debug_overlay", nullptr, _flags)) {
    ImGui::Text("Chunks: %zu loaded, %zu meshed, %zu meshing", session.world().chunkCount(), mesher.meshedChunks(), mesher.pendingChunks());

    if (const PlayerPrediction* player = session.player()) {
      const vec3     position = player->position();
      const ChunkPos chunk    = Chunk::posOf(ivec3(glm::floor(position)));

      ImGui::Text("XYZ: %.3f / %.3f / %.3f", position.x, position.y, position.z);
      ImGui::Text("Chunk: %d %d", chunk.x, chunk.y);
      ImGui::Text("Facing: yaw %.1f, pitch %.1f", glm::degrees(player->yaw()), glm::degrees(player->pitch()));
      ImGui::Text("Mode: %.*s", static_cast<int>(toString(player->player().mode()).size()), toString(player->player().mode()).data());
      ImGui::Text("Prediction: %zu pending, %.3f off", player->pending(), glm::length(player->correction()));
    }
  }
  ImGui::End();
}

}
