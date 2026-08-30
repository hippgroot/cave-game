#pragma once

#include "game.hpp"

#include "grf/device.hpp"
#include "grf/queue.hpp"

#include <stop_token>
#include <thread>
#include <unordered_map>

namespace grf {

class Swapchain;

}

class Assets {
  std::unordered_map<str, grf::Image>     m_textures;
  std::unordered_map<str, grf::Sampler>   m_samplers;
  std::unordered_map<str, grf::Shader>    m_shaders;
  std::unordered_map<str, grf::Pipeline>  m_pipelines;

  grf::Buffer m_textureIndexLUT;

public:
  Assets(const grf::Device& device, grf::Format swapchainFormat, grf::Format depthFormat);
  ~Assets() = default;

  Assets(const Assets&) = delete;
  Assets(Assets&&) = delete;
  Assets& operator=(const Assets&) = delete;
  Assets& operator=(Assets&&) = delete;

  grf::Image texture(const str& name) const;
  grf::Sampler sampler(const str& name) const;
  grf::Shader shader(const str& name) const;
  grf::Pipeline pipeline(const str& name) const;
  grf::Buffer textureLUT() const;

private:
  void loadTextures(const grf::Device& device);
  void createSamplers(const grf::Device& device);
  void compileShaders(const grf::Device& device);
  void createPipelines(const grf::Device& device, grf::Format swapchainFormat, grf::Format depthFormat);
};

class Graveyard {
  using ValidType = std::variant<grf::Buffer, grf::Image>;
  std::vector<std::pair<u32, ValidType>> m_retired;

public:
  Graveyard() = default;
  ~Graveyard() = default;

  Graveyard(const Graveyard&) = delete;
  Graveyard(Graveyard&&) = delete;
  Graveyard& operator=(const Graveyard&) = delete;
  Graveyard& operator=(Graveyard&&) = delete;

  void retire(ValidType resource, u32 frameDelay);
  void update();
};

class Renderer {
public:
  struct Info {
    const grf::Device&    device;
    const grf::Swapchain& swapchain;
    const grf::GUI&       gui;
    const grf::Profiler&  profiler;
    Channel<f32>&         beginFrame;
    Channel<Game::Data>&  newFrame;
  };

private:
  struct ChunkMesh {
    grf::Buffer vertexBuffer;
    grf::Buffer indexBuffer;
    grf::Buffer blocksBuffer;
    u32         indexCount;
  };

  using ChunkCache = std::unordered_map<ivec2, ChunkMesh, ivec2_hash>;

  static const u32              m_flightFrames = 2u;
  static const grf::Format      m_depthFormat = grf::Format::d32_sfloat;

  const grf::Device&            m_device;
  const grf::Swapchain&         m_swapchain;
  const grf::GUI&               m_gui;
  const grf::Profiler&          m_profiler;

  Channel<f32>&                 m_beginFrame;
  Channel<Game::Data>&          m_newFrame;

  Assets                        m_assets;
  Graveyard                     m_graveyard;
  ChunkCache                    m_chunkCache;

  grf::Queue                    m_queue;
  grf::Ring<grf::CommandBuffer> m_cmdRing;
  grf::Ring<grf::Sync>          m_syncRing;
  grf::Image                    m_depthImage;

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
  void updateChunkCache(const World::AddedChunks& addedChunks, const std::vector<ivec2>& removedChunks);
  void renderChunks(grf::CommandBuffer cmd, grf::SwapchainImage swapchainImage, const mat4& projView, const Camera::Frustrum& frustrum);
  void renderGUI(grf::CommandBuffer cmd, grf::SwapchainImage swapchainImage, grf::GUI::DrawData guiSnapshot);
};