#pragma once

#include "core/types.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <type_traits>
#include <unordered_map>

#define THREAD_BUS ThreadBus::instance()

using ChannelID = u64;

class BaseChannel {
public:
  virtual ~BaseChannel() = default;
};

template <typename T>
class Channel : public BaseChannel {
  std::mutex                  m_mutex;
  std::deque<T>               m_queue;
  std::condition_variable_any m_cv;

public:
  Channel()  = default;
  ~Channel() = default;

  Channel(const Channel&)            = delete;
  Channel(Channel&&)                 = delete;
  Channel& operator=(const Channel&) = delete;
  Channel& operator=(Channel&&)      = delete;

  void send(T data) {
    {
      std::lock_guard lock(m_mutex);
      m_queue.push_back(std::move(data));
    }
    m_cv.notify_one();
  }

  std::optional<T> receive() {
    std::lock_guard lock(m_mutex);
    if (m_queue.empty()) return std::nullopt;

    auto data = std::move(m_queue.front());
    m_queue.pop_front();

    return std::move(data);
  }

  std::optional<T> wait_to_receive(std::stop_token st, std::optional<std::chrono::milliseconds> timeout_ms) {
    std::unique_lock lock(m_mutex);

    auto ready = [this] { return !m_queue.empty(); };

    if (timeout_ms)
      m_cv.wait_for(lock, st, *timeout_ms, ready);
    else
      m_cv.wait(lock, st, [this] { return !m_queue.empty(); });

    if (m_queue.empty()) return std::nullopt;

    auto data = std::move(m_queue.front());
    m_queue.pop_front();

    return std::move(data);
  }
};

class ThreadBus {
  std::mutex                                                  m_mutex;
  std::unordered_map<ChannelID, std::shared_ptr<BaseChannel>> m_channels;
  ChannelID                                                   m_nextID = 0;

public:
  ~ThreadBus() = default;

  ThreadBus(const ThreadBus&)            = delete;
  ThreadBus(ThreadBus&&)                 = delete;
  ThreadBus& operator=(const ThreadBus&) = delete;
  ThreadBus& operator=(ThreadBus&&)      = delete;

  inline static ThreadBus& instance() {
    static ThreadBus _instance;
    return _instance;
  }

  template <typename T>
  ChannelID registerChannel() {
    std::lock_guard lock(m_mutex);

    ChannelID id = ++m_nextID;
    m_channels.emplace(id, std::make_shared<Channel<T>>());

    return id;
  }

  inline void unregisterChannel(ChannelID id) {
    std::lock_guard lock(m_mutex);

    auto itr = m_channels.find(id);
    if (itr != m_channels.end())
      m_channels.erase(itr);
  }

  template <typename T>
  void send(ChannelID id, std::type_identity_t<T> data) {
    auto channel = find<T>(id);
    if (channel == nullptr) return;
    channel->send(std::move(data));
  }

  template <typename T>
  std::optional<T> receive(ChannelID id) {
    auto channel = find<T>(id);
    return channel == nullptr ? std::nullopt : channel->receive();
  }

  template <typename T>
  std::optional<T> wait_to_receive(ChannelID id, std::stop_token st, std::optional<std::chrono::milliseconds> timeout_ms = std::nullopt) {
    auto channel = find<T>(id);
    return channel == nullptr ? std::nullopt : channel->wait_to_receive(st, timeout_ms);
  }

private:
  ThreadBus() = default;

  template <typename T>
  std::shared_ptr<Channel<T>> find(ChannelID id) {
    std::lock_guard lock(m_mutex);
    auto            itr = m_channels.find(id);
    return itr == m_channels.end() ? nullptr : std::dynamic_pointer_cast<Channel<T>>(itr->second);
  }
};
