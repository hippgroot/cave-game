#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <stop_token>

template <typename T>
class Channel {
  std::mutex                  m_mutex;
  std::condition_variable_any m_cv;
  std::deque<T>               m_packets;

public:
  Channel() = default;
  ~Channel() = default;

  Channel(const Channel&) = delete;
  Channel(Channel&&) = delete;
  Channel& operator=(const Channel&) = delete;
  Channel& operator=(Channel&&) = delete;

  void send(T packet) {
  {
    std::lock_guard lock(m_mutex);
    m_packets.push_back(std::move(packet));
  }
    m_cv.notify_one();
  }

  std::optional<T> receive() {
    std::lock_guard lock(m_mutex);
    if (m_packets.empty()) return std::nullopt;

    T packet = std::move(m_packets.front());
    m_packets.pop_front();

    return std::move(packet);
  }

  std::optional<T> block_and_receive(std::stop_token st) {
    std::unique_lock lock(m_mutex);

    m_cv.wait(lock, st, [this]{ return !m_packets.empty(); });
    if (m_packets.empty()) return std::nullopt;

    T packet = std::move(m_packets.front());
    m_packets.pop_front();

    return std::move(packet);
  }
};