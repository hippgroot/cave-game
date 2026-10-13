#pragma once

#include "core/threadbus.hpp"
#include "core/types.hpp"
#include "engine/assets.hpp"
#include "engine/camera.hpp"
#include "engine/graveyard.hpp"
#include "engine/mesh.hpp"

#include <grf/gui.hpp>
#include <grf/pipeline.hpp>
#include <grf/queue.hpp>
#include <grf/resources.hpp>

#include <optional>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>

namespace grf {

class Device;
class Swapchain;
class SwapchainImage;

}

class Renderer {
public:
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
    ChannelID             beginFrameChannel;
    ChannelID             renderFrameChannel;
  };

private:
  struct Mesh {
    grf::Buffer                vertexBuffer;
    grf::Buffer                indexBuffer;
    std::optional<grf::Buffer> auxBuffer;
    std::vector<byte>          pushConstants;
    AABB                       aabb;
    u32                        indexCount;
    u8                         layer;
    u8                         pipeline;
  };

  struct Globals {
    u64 camBuf;
    u64 textureLUT;
    u32 nearestClampSampler;
    f32 dt;
  };

  using MeshCache    = std::unordered_map<MeshID, Mesh>;
  using RenderID     = u64;
  using RenderBuffer = std::vector<RenderID>;

  static constexpr u32         m_flightFrames = 2u;
  static constexpr grf::Format m_depthFormat  = grf::Format::d32_sfloat;

  const grf::Device&    m_device;
  const grf::Swapchain& m_swapchain;
  const grf::GUI&       m_gui;
  const grf::Profiler&  m_profiler;

  Assets                  m_assets;
  Registry<grf::Pipeline> m_pipelines;
  grf::Sampler            m_nearestClamp;
  Graveyard               m_graveyard;
  MeshCache               m_meshCache;
  grf::Image              m_depthImage;
  grf::Buffer             m_camBuf;
  Ring<grf::Buffer>       m_globRing;
  Camera::Frustum         m_cameraFrustum;
  vec3                    m_cameraPosition = vec3(0.0f);

  grf::Queue               m_queue;
  Ring<grf::CommandBuffer> m_cmdRing;
  Ring<grf::Sync>          m_syncRing;

  ChannelID m_beginFrameChannel;
  ChannelID m_renderFrameChannel;

  std::jthread m_thread;

public:
  explicit Renderer(const Info& info);
  ~Renderer();

  Renderer(const Renderer&)            = delete;
  Renderer(Renderer&&)                 = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer& operator=(Renderer&&)      = delete;

private:
  void createPipelines();
  void loop(std::stop_token st);

  void updateCameraBuffer(Camera::UpdateData updateData);
  void updateGlobalsBuffer(grf::Buffer globBuf, f32 dt);
  void unloadMeshes(std::vector<MeshID> ids);
  void cacheMeshes(std::vector<MeshData> newMeshes);

  RenderBuffer constructRenderBuffer() const;
  void         render(grf::CommandBuffer cmd, grf::SwapchainImage swapchainImage, grf::Buffer globBuf, const RenderBuffer& rb) const;
  void         renderGUI(grf::CommandBuffer cmd, grf::SwapchainImage swapchainImage, grf::GUI::DrawData guiSnapshot) const;
};
