#include "engine/renderer.hpp"

#include "core/log.hpp"
#include "core/threadbus.hpp"

#include <grf/device.hpp>
#include <grf/swapchain.hpp>

#include <algorithm>
#include <span>

using namespace grf;

Renderer::Renderer(const Info& info)
: m_device(info.device),
  m_swapchain(info.swapchain),
  m_gui(info.gui),
  m_profiler(info.profiler),
  m_assets(m_device),
  m_beginFrameChannel(info.beginFrameChannel),
  m_renderFrameChannel(info.renderFrameChannel) {
  m_queue    = m_device.graphicsQueue();
  m_cmdRing  = m_queue.getCmdRing(m_flightFrames);
  m_syncRing = m_queue.getSyncRing(m_flightFrames);

  m_nearestClamp = m_device.createSampler(Sampler::Info{
    .magFilter    = Filter::Nearest,
    .minFilter    = Filter::Nearest,
    .addressModeU = AddressMode::ClampToBorder,
    .addressModeV = AddressMode::ClampToBorder,
    .addressModeW = AddressMode::ClampToBorder
  });

  createPipelines();

  m_depthImage = m_device.createImage(Image::Info{
    .width  = 1u,
    .height = 1u,
    .format = m_depthFormat,
    .usage  = ImageUsage::DepthAttachment
  });

  m_camBuf = m_device.createBuffer(Buffer::Info{
    .size   = sizeof(Camera::Data),
    .intent = BufferIntent::GPUOnly
  });

  m_globRing = m_device.createBufferRing(m_flightFrames, Buffer::Info{
                                                           .size   = sizeof(Globals),
                                                           .intent = BufferIntent::FrequentUpdate
                                                         });

  m_thread = std::jthread([this](std::stop_token st) { loop(st); });
}

Renderer::~Renderer() {
  m_thread.request_stop();
  if (m_thread.joinable())
    m_thread.join();
}

void Renderer::createPipelines() {
  const auto& shaders = m_assets.shaders();

  const auto createGraphics = [this](const str& name, std::initializer_list<Shader> stages, const GraphicsPipelineSettings& settings) {
    m_pipelines.add(name, m_device.createGraphicsPipeline(std::span(stages.begin(), stages.size()), settings));
  };

  createGraphics("chunk_draw", {shaders["chunk_draw/chunk.vert"], shaders["chunk_draw/chunk.frag"]},
    GraphicsPipelineSettings{
      .colorFormats = {m_swapchain.format()},
      .depthFormat  = m_depthFormat,
      .depthTest    = true,
      .depthWrite   = true,
      .blends       = {
        BlendState{
          .enable         = true,
          .srcColorFactor = BlendFactor::SrcAlpha,
          .dstColorFactor = BlendFactor::OneMinusSrcAlpha
        }
      }
    });

  createGraphics("chunk_translucent", {shaders["chunk_draw/chunk.vert"], shaders["chunk_draw/chunk.frag"]},
    GraphicsPipelineSettings{
      .colorFormats = {m_swapchain.format()},
      .depthFormat  = m_depthFormat,
      .cullMode     = CullMode::None,
      .depthTest    = true,
      .depthWrite   = false,
      .blends       = {
        BlendState{
          .enable         = true,
          .srcColorFactor = BlendFactor::SrcAlpha,
          .dstColorFactor = BlendFactor::OneMinusSrcAlpha
        }
      }
    });
}

void Renderer::loop(std::stop_token st) {
  logging::ThreadScope thread("render");

  f32 prevDt = 0.0f;
  THREAD_BUS.send<f32>(m_beginFrameChannel, prevDt);

  while (auto frameopt = THREAD_BUS.wait_to_receive<FrameData>(m_renderFrameChannel, st)) {
    FrameData frameData = std::move(*frameopt);

    auto& globBuf   = m_globRing.next();
    auto& cmd       = m_cmdRing.next();
    auto& frameSync = m_syncRing.next();

    m_device.wait({frameSync});
    f32 dt = m_device.beginFrame();
    THREAD_BUS.send<f32>(m_beginFrameChannel, dt);
    m_graveyard.update();

    auto [swapchainImage, imgSync] = m_swapchain.acquireNextImage();
    if (m_depthImage.width() != swapchainImage.width() || m_depthImage.height() != swapchainImage.height()) {
      m_graveyard.retire(m_depthImage, m_flightFrames);
      m_depthImage = m_device.createImage(Image::Info{
        .width  = swapchainImage.width(),
        .height = swapchainImage.height(),
        .format = m_depthFormat,
        .usage  = ImageUsage::DepthAttachment
      });
    }

    if (frameData.cameraData)
      updateCameraBuffer(std::move(*frameData.cameraData));

    updateGlobalsBuffer(globBuf, prevDt);
    prevDt = dt;

    unloadMeshes(std::move(frameData.removedMeshes));
    cacheMeshes(std::move(frameData.addedMeshes));

    RenderBuffer rb = constructRenderBuffer();
    std::ranges::sort(rb);

    cmd.begin();
    m_profiler.beginFrame(cmd);

    render(cmd, swapchainImage, globBuf, rb);
    renderGUI(cmd, swapchainImage, std::move(frameData.guiSnapshot));

    cmd.end();

    Sync resourceSync = m_device.updateResources();

    frameSync = m_queue.submit({cmd}, {imgSync, resourceSync});
    m_queue.present(swapchainImage, {frameSync});
  }
  m_device.waitIdle();
}

void Renderer::updateCameraBuffer(Camera::UpdateData updateData) {
  auto [frustum, data] = std::move(updateData);

  m_graveyard.retire(m_camBuf, m_flightFrames);
  m_camBuf = m_device.createBuffer(Buffer::Info{
    .size   = sizeof(Camera::Data),
    .intent = BufferIntent::GPUOnly
  });
  m_camBuf.write(data);

  m_cameraFrustum  = std::move(frustum);
  m_cameraPosition = data.position;
}

void Renderer::updateGlobalsBuffer(Buffer globBuf, f32 dt) {
  globBuf.write(Globals{
    .camBuf              = m_camBuf.address(),
    .textureLUT          = m_assets.textureLUT().address(),
    .nearestClampSampler = m_nearestClamp.heapIndex(),
    .dt                  = dt
  });
}

void Renderer::unloadMeshes(std::vector<MeshID> ids) {
  for (auto id : ids) {
    if (!m_meshCache.contains(id)) continue;
    Mesh& mesh = m_meshCache.at(id);

    m_graveyard.retire(mesh.vertexBuffer, m_flightFrames);
    m_graveyard.retire(mesh.indexBuffer, m_flightFrames);
    if (mesh.auxBuffer)
      m_graveyard.retire(*mesh.auxBuffer, m_flightFrames);

    m_meshCache.erase(id);
  }
}

void Renderer::cacheMeshes(std::vector<MeshData> newMeshes) {
  for (auto& data : newMeshes) {
    Buffer vertexBuffer = m_device.createBuffer(Buffer::Info{
      .size   = static_cast<u32>(data.vertices.size()),
      .intent = BufferIntent::GPUOnly
    });
    vertexBuffer.write(data.vertices);

    Buffer indexBuffer = m_device.createBuffer(Buffer::Info{
      .size   = static_cast<u32>(data.indices.size() * sizeof(u32)),
      .intent = BufferIntent::GPUOnly
    });
    indexBuffer.write(data.indices);

    std::optional<Buffer> auxBuffer = std::nullopt;
    if (!data.aux.empty()) {
      auxBuffer = m_device.createBuffer(Buffer::Info{
        .size   = static_cast<u32>(data.aux.size()),
        .intent = BufferIntent::GPUOnly
      });
      auxBuffer->write(data.aux);
    }

    u8 pipeline = static_cast<u8>(m_pipelines.indexOf(data.pipeline));

    m_meshCache.emplace(data.meshID, Mesh{
                                       .vertexBuffer  = vertexBuffer,
                                       .indexBuffer   = indexBuffer,
                                       .auxBuffer     = auxBuffer,
                                       .pushConstants = std::move(data.pushConstants),
                                       .aabb          = data.aabb,
                                       .indexCount    = static_cast<u32>(data.indices.size()),
                                       .layer         = data.layer,
                                       .pipeline      = pipeline
                                     });
  }
}

Renderer::RenderBuffer Renderer::constructRenderBuffer() const {
  RenderBuffer renderBuffer;
  renderBuffer.reserve(m_meshCache.size());

  for (const auto& [meshID, mesh] : m_meshCache) {
    if (!m_cameraFrustum.intersects(mesh.aabb.min, mesh.aabb.max)) continue;

    vec3 meshCenter = (mesh.aabb.min + mesh.aabb.max) / 2.0f;
    f32  depth      = glm::distance(m_cameraPosition, meshCenter) / Camera::farPlane();

    u32 top12 = ((static_cast<u32>(mesh.layer) << 8) | static_cast<u32>(mesh.pipeline)) << 20;

    u32 depthBits = static_cast<u32>(
      std::clamp(depth, 0.0f, 1.0f) * ((1 << 20) - 1));
    if (mesh.layer & 0x8)
      depthBits = ~depthBits & 0xFFFFF;

    u64 topBits = static_cast<u64>(top12 | depthBits) << 32;

    renderBuffer.emplace_back(topBits | static_cast<u64>(meshID));
  }

  return renderBuffer;
}

void Renderer::render(CommandBuffer cmd, SwapchainImage swapchainImage, Buffer globBuf, const RenderBuffer& rb) const {
  struct PCHeader {
    u64 auxBuffer;
    u64 vertexBuffer;
    u64 globalsBuffer;
  };

  cmd.transition(swapchainImage, Layout::ColorAttachmentOptimal);
  cmd.transition(m_depthImage, Layout::DepthAttachmentOptimal);

  cmd.beginRendering(swapchainImage.width(), swapchainImage.height(),
    {ColorAttachment{swapchainImage, LoadOp::Clear, StoreOp::Store, {0.1f, 0.2f, 0.5f, 1.0f}}},
    DepthAttachment{m_depthImage, LoadOp::Clear, StoreOp::DontCare});

  u8 currentPipeline = 0xFF;
  for (RenderID rid : rb) {
    u8  pipeline = (rid >> 52) & 0xFF;
    u32 meshID   = rid & 0xFFFFFFFF;

    const Mesh& mesh = m_meshCache.at(meshID);

    PCHeader header{
      .auxBuffer     = mesh.auxBuffer ? mesh.auxBuffer->address() : u64{0},
      .vertexBuffer  = mesh.vertexBuffer.address(),
      .globalsBuffer = globBuf.address()
    };
    auto headerBytes = std::as_bytes(std::span{&header, 1});

    std::vector<byte> pushConstants(headerBytes.begin(), headerBytes.end());
    pushConstants.insert(pushConstants.end(), mesh.pushConstants.begin(), mesh.pushConstants.end());

    if (currentPipeline != pipeline) {
      if (currentPipeline != 0xFF)
        m_profiler.endScope(cmd);
      currentPipeline = pipeline;
      m_profiler.beginScope(cmd, m_pipelines.nameOf(currentPipeline));
      cmd.bindPipeline(m_pipelines[currentPipeline]);
    }

    cmd.pushConstants(pushConstants);
    cmd.bindIndexBuffer(mesh.indexBuffer);
    cmd.drawIndexed(mesh.indexCount);
  }

  if (currentPipeline != 0xFF)
    m_profiler.endScope(cmd);

  cmd.endRendering();
}

void Renderer::renderGUI(CommandBuffer cmd, SwapchainImage swapchainImage, GUI::DrawData guiSnapshot) const {
  cmd.beginRendering(swapchainImage.width(), swapchainImage.height(),
    {ColorAttachment{swapchainImage, LoadOp::Load, StoreOp::Store}});

  m_profiler.beginScope(cmd, "gui");
  m_gui.render(cmd, guiSnapshot);
  m_profiler.endScope(cmd);

  cmd.endRendering();
}
