#pragma once

#include "core.hpp"

#include "grf/device.hpp"
#include "grf/types.hpp"

#include <stop_token>
#include <thread>

namespace grf {
  class Swapchain;
  class GUI;
  class Profiler;
}

using namespace grf;

class Renderer {
public:
  struct Info {
    const Device&       device;
    const Swapchain&    swapchain;
    const GUI&          gui;
    const Profiler&     profiler;
    Channel<f32>&       beginFrame;
    Channel<FrameData>& newFrame;
  };

private:
  static const u32    m_flightFrames = 2u;

  const Device&       m_device;
  const Swapchain&    m_swapchain;
  const GUI&          m_gui;
  const Profiler&     m_profiler;
  Channel<f32>&       m_beginFrame;
  Channel<FrameData>& m_newFrame;

  Queue               m_queue;
  Ring<CommandBuffer> m_cmdRing;
  Ring<Sync>          m_syncRing;

  std::jthread        m_thread;

public:
  explicit Renderer(const Info& info);
  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer& operator=(Renderer&&) = delete;

private:
  void loop(std::stop_token st);
};