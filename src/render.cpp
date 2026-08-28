#include "render.hpp"

#include <filesystem>

using namespace grf;
namespace fs = std::filesystem;

Assets::Assets(const Device& device, Format swapchainFormat) {
  compileShaders(device);
  createPipelines(device, swapchainFormat);
}

Shader Assets::shader(const str& name) const {
  return m_shaders.at(name);
}

Pipeline Assets::pipeline(const str& name) const {
  return m_pipelines.at(name);
}

void Assets::compileShaders(const Device& device) {
  const fs::path _dir = SHADER_DIR;
  const auto compile = [this, &device](ShaderStage stage, const fs::path& path){
    m_shaders.emplace(path.filename().string(), device.compileGlslShader(stage, path));
  };

  logging::general("Compiling shaders...");

  compile(ShaderStage::Vertex, _dir/"block.vert");
  compile(ShaderStage::Fragment, _dir/"block.frag");
}

void Assets::createPipelines(const Device& device, Format swapchainFormat) {
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

  createGraphics("block_draw", { shader("block.vert"), shader("block.frag") },
    GraphicsPipelineSettings{
      .colorFormats = { swapchainFormat },
      .depthFormat  = Format::d32_sfloat,
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
  m_assets(m_device, m_swapchain.format())
{
  m_queue     = m_device.graphicsQueue();
  m_cmdRing   = m_queue.getCmdRing(m_flightFrames);
  m_syncRing  = m_queue.getSyncRing(m_flightFrames);

  m_thread    = std::jthread([this](std::stop_token st){ loop(st); });
}

void Renderer::loop(std::stop_token st) {
  logging::register_thread(logging::Thread::render);
  m_beginFrame.send(0.0f);

  Buffer vertexBuffer;
  Buffer indexBuffer;
  u32 indexCount = 0;
{
  std::array<vec4, 8> vertices = {
    vec4(-0.5f, -0.5f, -0.5f, 1.0f),
    vec4(-0.5f, -0.5f, 0.5f, 1.0f),
    vec4(-0.5f, 0.5f, -0.5f, 1.0f),
    vec4(-0.5f, 0.5f, 0.5f, 1.0f),
    vec4(0.5f, -0.5f, -0.5f, 1.0f),
    vec4(0.5f, -0.5f, 0.5f, 1.0f),
    vec4(0.5f, 0.5f, -0.5f, 1.0f),
    vec4(0.5f, 0.5f, 0.5f, 1.0f)
  };

  std::array<u16, 36> indices = {
    0, 1, 3, 0, 3, 2,
    4, 6, 7, 4, 7, 5,
    0, 4, 5, 0, 5, 1,
    2, 3, 7, 2, 7, 6,
    0, 2, 6, 0, 6, 4,
    1, 5, 7, 1, 7, 3
  };

  vertexBuffer = m_device.createBuffer(Buffer::Info{
    .size   = sizeof(vec4) * vertices.size(),
    .intent = BufferIntent::GPUOnly
  });
  vertexBuffer.write(vertices);

  indexBuffer = m_device.createBuffer(Buffer::Info{
    .size   = sizeof(u16) * indices.size(),
    .intent = BufferIntent::GPUOnly
  });
  indexBuffer.write(indices);
  indexCount = indices.size();
}

  struct PC {
    u64               vertBufAddr;
    alignas(16) mat4  projView;
  };
  while (auto frameopt = m_newFrame.wait_to_receive(st)) {
    auto frame = std::move(*frameopt);
    auto& cmd = m_cmdRing.next();
    auto& frameSync = m_syncRing.next();

    m_device.wait({ frameSync });
    f32 dt = m_device.beginFrame();
    m_beginFrame.send(dt);
    m_graveyard.update();

    auto [swapchainImage, imgSync] = m_swapchain.acquireNextImage();
    Image depthImage = m_device.createImage(Image::Info{
      .width  = swapchainImage.width(),
      .height = swapchainImage.height(),
      .format = Format::d32_sfloat,
      .usage  = ImageUsage::DepthAttachment
    });

    cmd.begin();
    m_profiler.beginFrame(cmd);

    cmd.transition(swapchainImage, Layout::ColorAttachmentOptimal);
    cmd.transition(depthImage, Layout::DepthAttachmentOptimal);

    cmd.beginRendering(swapchainImage.width(), swapchainImage.height(),
      {{ swapchainImage, LoadOp::Clear, StoreOp::Store }},
      DepthAttachment{ depthImage, LoadOp::Clear, StoreOp::DontCare }
    );

    PC pc{ vertexBuffer.address(), frame.cameraProjView };
    cmd.bindPipeline(m_assets.pipeline("block_draw"));
    cmd.pushConstants(std::as_bytes(std::span{&pc, 1}));
    cmd.bindIndexBuffer(indexBuffer, IndexType::u16);
    m_profiler.beginScope(cmd, "block draw");
    cmd.drawIndexed(indexCount);
    m_profiler.endScope(cmd);

    cmd.endRendering();
    cmd.beginRendering(swapchainImage.width(), swapchainImage.height(),
      {{ swapchainImage, LoadOp::Load, StoreOp::Store }}
    );

    m_profiler.beginScope(cmd, "gui");
    m_gui.render(cmd, frame.guiSnapshot);
    m_profiler.endScope(cmd);

    cmd.endRendering();
    cmd.transition(swapchainImage, Layout::PresentSrc);
    cmd.end();

    Sync resourceSync = m_device.updateResources();

    frameSync = m_queue.submit({ cmd }, { imgSync, resourceSync });
    m_queue.present(swapchainImage, { frameSync });

    m_graveyard.retire(depthImage, m_flightFrames);
  }

  m_device.waitIdle();
}