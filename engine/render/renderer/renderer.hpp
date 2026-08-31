#pragma once

#include "../assets/assets.hpp"
#include "../camera/camera.hpp"
#include "../graveyard/graveyard.hpp"

#include "grf/device.hpp"
#include "grf/gui.hpp"

#include <thread>

namespace grf {
class Swapchain;
}

class Renderer {
public:
  using MeshID = u32;

  struct FrameData {
    grf::GUI::DrawData                guiSnapshot;
    std::optional<Camera::UpdateData> cameraData;
    std::vector<MeshData>             addedMeshes;
    std::vector<MeshID>               removedMeshes;
  };

  struct Info {
    const grf::Device&    device;
    const grf::Swapchain& swapchain;
    const grf::GUI&       gui;
    const grf::Profiler&  profiler;
    Channel<FrameData>&   newFrame;
    Channel<f32>&         beginFrame;
  };

  struct Mesh {
    grf::Buffer                 vertexBuffer;
    grf::Buffer                 indexBuffer;
    std::optional<grf::Buffer>  auxBuffer;
    std::vector<std::byte>      pushConstants;
    AABB                        aabb;
    u32                         indexCount;
    u8                          layer;
    u8                          pipeline;
  };

private:
  struct Globals {
    u64 camBuf;
    u64 textureLUT;
    u32 nearestClampSampler;
    f32 dt;
  };

  using MeshCache = std::unordered_map<MeshID, Mesh>;
  using RenderID = u64;
  using RenderBuffer = std::vector<RenderID>;

  static constexpr u32          m_flightFrames = 2u;
  static constexpr grf::Format  m_depthFormat = grf::Format::d32_sfloat;

  const grf::Device&            m_device;
  const grf::Swapchain&         m_swapchain;
  const grf::GUI&               m_gui;
  const grf::Profiler&          m_profiler;

  Channel<FrameData>&           m_newFrame;
  Channel<f32>&                 m_beginFrame;

  Assets                        m_assets;
  Graveyard                     m_graveyard;
  MeshCache                     m_meshCache;
  grf::Image                    m_depthImage;
  grf::Buffer                   m_camBuf;
  grf::Ring<grf::Buffer>        m_globRing;
  Camera::Frustum               m_cameraFrustum;
  vec3                          m_cameraPosition;

  grf::Queue                    m_queue;
  grf::Ring<grf::CommandBuffer> m_cmdRing;
  grf::Ring<grf::Sync>          m_syncRing;

  std::jthread                  m_thread;

public:
  explicit Renderer(const Info& info);
  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer& operator=(Renderer&&) = delete;

private:
  void loop(std::stop_token st);

  void updateCameraBuffer(Camera::UpdateData updateData);
  void updateGlobalsBuffer(grf::Buffer globBuf, f32 dt);
  void unloadMeshes(std::vector<MeshID> ids);
  void cacheMeshes(std::vector<MeshData> newMeshes);

  RenderBuffer constructRenderBuffer() const;
  void render(grf::CommandBuffer cmd, grf::SwapchainImage swapchainImage, grf::Buffer globBuf, const RenderBuffer& rb) const;
  void renderGUI(grf::CommandBuffer cmd, grf::SwapchainImage swapchainImage, grf::GUI::DrawData guiSnapshot) const;
};