#include "render.hpp"
#include "core.hpp"

Renderer::Renderer(const Info& info)
: m_device(info.device),
  m_swapchain(info.swapchain),
  m_gui(info.gui),
  m_profiler(info.profiler),
  m_beginFrame(info.beginFrame),
  m_newFrame(info.newFrame)
{
  m_queue     = m_device.graphicsQueue();
  m_cmdRing   = m_queue.getCmdRing(m_flightFrames);
  m_syncRing  = m_queue.getSyncRing(m_flightFrames);

  m_thread    = std::jthread([this](std::stop_token st){ loop(st); });
}

void Renderer::loop(std::stop_token st) {
  logging::register_thread(logging::Thread::render);
  m_beginFrame.send(0.0f);

  while (std::optional<FrameData> frameopt = m_newFrame.wait_to_receive(st)) {
    FrameData frame = std::move(*frameopt);
    auto& cmd = m_cmdRing.next();
    auto& frameSync = m_syncRing.next();

    m_device.wait({ frameSync });
    f32 dt = m_device.beginFrame();
    m_beginFrame.send(dt);

    auto [swapchainImage, imgSync] = m_swapchain.acquireNextImage();

    cmd.begin();
    m_profiler.beginFrame(cmd);
    cmd.transition(swapchainImage, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(swapchainImage.width(), swapchainImage.height(),
      {{ swapchainImage, LoadOp::Clear, StoreOp::Store }}
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
  }

  m_device.waitIdle();
}