#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <unordered_map>

#define THREAD_BUS ThreadBus::instance()

using ChannelID = std::uint64_t;

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

  std::optional<T> wait_to_receive(std::stop_token st) {
    std::unique_lock lock(m_mutex);

    m_cv.wait(lock, st, [this] { return !m_queue.empty(); });
    if (m_queue.empty()) return std::nullopt;

    auto data = std::move(m_queue.front());
    m_queue.pop_front();

    return std::move(data);
  }
};

class ThreadBus {
  std::mutex                                                  m_mutex;
  std::unordered_map<ChannelID, std::unique_ptr<BaseChannel>> m_channels;
  std::unordered_map<std::string, ChannelID>                  m_tagMap;

public:
  ~ThreadBus() = default;

  static ThreadBus& instance() {
    static ThreadBus _instance;
    return _instance;
  }

  template <typename T>
  ChannelID registerChannel() {
    std::lock_guard lock(m_mutex);

    static ChannelID _nextID = 0;
    m_channels.emplace(++_nextID, std::make_unique<Channel<T>>());

    return _nextID;
  }

  inline void unregisterChannel(ChannelID id) {
    std::lock_guard lock(m_mutex);

    auto itr = m_channels.find(id);
    if (itr == m_channels.end()) return;

    std::erase_if(m_tagMap, [&](auto elem) {
      return elem.second == itr->first;
    });
    m_channels.erase(itr);
  }

  inline void tagChannel(ChannelID id, const std::string& tag) {
    std::lock_guard lock(m_mutex);

    if (m_channels.find(id) == m_channels.end()) return;
    m_tagMap.emplace(tag, id);
  }

  inline ChannelID channelID(const std::string& tag) const {
    std::lock_guard lock(m_mutex);

    auto itr = m_tagMap.find(tag);
    return itr == m_tagMap.end() ? 0 : itr->second;
  }

  template <typename T>
  Channel<T>* channel(ChannelID id) {
    std::lock_guard lock(m_mutex);

    auto itr = m_channels.find(id);
    return itr == m_channels.end() ? nullptr : dynamic_cast<Channel<T>*>(itr->second.get());
  }

private:
  ThreadBus() = default;
};
