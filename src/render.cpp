#include "render.hpp"

#include <filesystem>

using namespace grf;
namespace fs = std::filesystem;

Assets::Assets(const Device& device, Format swapchainFormat, Format depthFormat) {
  loadTextures(device);
  createSamplers(device);
  compileShaders(device);
  createPipelines(device, swapchainFormat, depthFormat);
}

Image Assets::texture(const str& name) const {
  return m_textures.at(name);
}

Sampler Assets::sampler(const str& name) const {
  return m_samplers.at(name);
}

Shader Assets::shader(const str& name) const {
  return m_shaders.at(name);
}

Pipeline Assets::pipeline(const str& name) const {
  return m_pipelines.at(name);
}

void Assets::loadTextures(const Device& device) {
  const fs::path _dir = fs::path(ASSETS_DIR)/"textures";
  const auto load = [this, &device, &_dir](const str& filename){
    fs::path path = _dir/filename;
    str name = path.stem().string();
    Image::Data data = Image::getDataFromFile(path);

    Image image = device.createImage(Image::Info{
      .width  = data.width,
      .height = data.height,
      .format = data.format,
      .usage  = ImageUsage::Sampled
    });
    image.write(data.bytes, Layout::ShaderReadOptimal);

    m_textures.emplace(name, image);
    logging::general("- {} ({})", name, image.sampledHeapIndex());
  };

  logging::general("Loading Textures...");

  load("default.png");
  load("grass_block.png");
  load("dirt_block.png");
  load("stone_block.png");
  load("bedrock_block.png");
}

void Assets::createSamplers(const Device& device) {
  const auto createSampler = [this, &device](const str& name, const Sampler::Info& info){
    Sampler s = device.createSampler(info);

    m_samplers.emplace(name, s);
    logging::general("- {} ({})", name, s.heapIndex());
  };

  logging::general("Creating Samplers...");

  createSampler("nearest_repeat", Sampler::Info{
    .magFilter = Filter::Nearest,
    .minFilter = Filter::Nearest
  });
}

void Assets::compileShaders(const Device& device) {
  const fs::path _dir = fs::path(ASSETS_DIR)/"shaders";
  const auto compile = [this, &device](ShaderStage stage, const fs::path& path){
    m_shaders.emplace(path.filename().string(), device.compileGlslShader(stage, path));
  };

  logging::general("Compiling shaders...");

  compile(ShaderStage::Vertex, _dir/"chunk_draw"/"chunk.vert");
  compile(ShaderStage::Fragment, _dir/"chunk_draw"/"chunk.frag");
}

void Assets::createPipelines(const Device& device, Format swapchainFormat, Format depthFormat) {
  const auto createGraphics = [this, &device, &swapchainFormat](
    const str& name,
    std::initializer_list<Shader> shaders,
    GraphicsPipelineSettings settings
  )
  {
    m_pipelines.emplace(
      name,
      device.createGraphicsPipeline(std::span<const Shader>(shaders.begin(), shaders.size()), settings)
    );
    logging::general("- {}", name);
  };

  const auto createCompute = [this, &device](const str& name, Shader shader) {
    m_pipelines.emplace(name, device.createComputePipeline(shader));
    logging::general("- {}", name);
  };

  logging::general("Creating pipelines...");

  createGraphics("chunk_draw", { shader("chunk.vert"), shader("chunk.frag") },
    GraphicsPipelineSettings{
      .colorFormats = { swapchainFormat },
      .depthFormat  = depthFormat,
      .cullMode     = CullMode::None,
      .depthTest    = true,
      .depthWrite   = true,
      .blends = {
        BlendState{
          .enable         = true,
          .srcColorFactor = BlendFactor::SrcAlpha,
          .dstColorFactor = BlendFactor::OneMinusSrcAlpha
        }
      }
    }
  );
}

void Graveyard::retire(ValidType resource, u32 frameDelay) {
  m_retired.emplace_back(frameDelay, resource);
}

void Graveyard::update() {
  for (auto& [f, r] : m_retired) --f;
  std::erase_if(m_retired, [](auto r){ return r.first == 0; });
}

Renderer::Renderer(const Info& info)
: m_device(info.device),
  m_swapchain(info.swapchain),
  m_gui(info.gui),
  m_profiler(info.profiler),
  m_beginFrame(info.beginFrame),
  m_newFrame(info.newFrame),
  m_assets(m_device, m_swapchain.format(), m_depthFormat)
{
  m_queue     = m_device.graphicsQueue();
  m_cmdRing   = m_queue.getCmdRing(m_flightFrames);
  m_syncRing  = m_queue.getSyncRing(m_flightFrames);

  m_thread    = std::jthread([this](std::stop_token st){ loop(st); });
}

void Renderer::loop(std::stop_token st) {
  logging::register_thread(logging::Thread::render);
  m_beginFrame.send(0.0f);

  while (auto dataopt = m_newFrame.wait_to_receive(st)) {
    auto gameData = std::move(*dataopt);
    auto& cmd = m_cmdRing.next();
    auto& frameSync = m_syncRing.next();

    m_device.wait({ frameSync });
    f32 dt = m_device.beginFrame();
    m_beginFrame.send(dt);
    m_graveyard.update();

    updateChunkCache(gameData.addedChunks, gameData.removedChunks);

    auto [swapchainImage, imgSync] = m_swapchain.acquireNextImage();

    cmd.begin();
    m_profiler.beginFrame(cmd);

    cmd.transition(swapchainImage, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(swapchainImage.width(), swapchainImage.height(),
      {{ swapchainImage, LoadOp::Clear, StoreOp::Store }}
    );

    m_profiler.beginScope(cmd, "gui");
    m_gui.render(cmd, gameData.guiSnapshot);
    m_profiler.endScope(cmd);

    cmd.endRendering();
    cmd.transition(swapchainImage, Layout::PresentSrc);
    cmd.end();

    Sync resourceSync = m_device.updateResources();

    frameSync = m_queue.submit({ cmd }, { imgSync, resourceSync });
    m_queue.present(swapchainImage, { frameSync });
  }

  m_device.waitIdle();
}

void Renderer::updateChunkCache(const World::AddedChunks& addedChunks, const std::vector<ivec2>& removedChunks) {
  for (auto coord : removedChunks) {
    ChunkMesh& m = m_chunkCache.at(coord);

    m_graveyard.retire(m.vertexBuffer, m_flightFrames);
    m_graveyard.retire(m.indexBuffer, m_flightFrames);
    m_graveyard.retire(m.blocksBuffer, m_flightFrames);

    m_chunkCache.erase(coord);
  }

  for (const auto& [coord, meshData] : addedChunks) {
    Buffer vertexBuffer = m_device.createBuffer(Buffer::Info{
      .size   = static_cast<u32>(meshData.vertices.size() * sizeof(Chunk::Vertex)),
      .intent = BufferIntent::GPUOnly
    });
    vertexBuffer.write(meshData.vertices);

    Buffer indexBuffer = m_device.createBuffer(Buffer::Info{
      .size = static_cast<u32>(meshData.indices.size() * sizeof(u32)),
      .intent = BufferIntent::GPUOnly
    });
    indexBuffer.write(meshData.indices);

    Buffer blocksBuffer = m_device.createBuffer(Buffer::Info{
      .size   = static_cast<u32>(meshData.types.size() * sizeof(u32)),
      .intent = BufferIntent::GPUOnly
    });
    blocksBuffer.write(meshData.types);

    m_chunkCache.emplace(coord, ChunkMesh{
      .vertexBuffer = vertexBuffer,
      .indexBuffer  = indexBuffer,
      .blocksBuffer = blocksBuffer,
      .indexCount   = static_cast<u32>(meshData.indices.size())
    });
  }
}