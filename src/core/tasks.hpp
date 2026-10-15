#pragma once

#include "core/log.hpp"
#include "core/types.hpp"

#include <algorithm>
#include <condition_variable>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

#define SCHEDULER Scheduler::instance()

using TaskID = i64;

enum class TaskStatus : u8 {
  Submitted,
  InProgress,
  Completed,
  Cancelled
};

class Scheduler {
  using Job = std::function<void()>;

  std::mutex                  m_mutex;
  std::map<TaskID, Job>       m_queue;
  std::condition_variable_any m_cv;
  u32                         m_nextSequence = 0;
  std::vector<std::jthread>   m_threads;

public:
  ~Scheduler() = default;

  Scheduler(const Scheduler&)            = delete;
  Scheduler(Scheduler&&)                 = delete;
  Scheduler& operator=(const Scheduler&) = delete;
  Scheduler& operator=(Scheduler&&)      = delete;

  inline static Scheduler& instance() {
    static Scheduler _instance;
    return _instance;
  }

  inline u32 workerCount() const { return static_cast<u32>(m_threads.size()); }

  inline TaskID submit(i32 priority, Job job) {
    TaskID id = 0;
    {
      std::lock_guard lock(m_mutex);
      id = enqueue(priority, std::move(job));
    }
    m_cv.notify_one();
    return id;
  }

  inline std::optional<TaskID> updatePriority(TaskID id, i32 priority) {
    std::lock_guard lock(m_mutex);

    auto node = m_queue.extract(id);
    if (node.empty()) return std::nullopt;

    return enqueue(priority, std::move(node.mapped()));
  }

  inline bool withdraw(TaskID id) {
    std::lock_guard lock(m_mutex);
    return m_queue.erase(id) > 0;
  }

private:
  inline Scheduler() {
    const i32 hardware = static_cast<i32>(std::thread::hardware_concurrency());
    const i32 count    = std::max(1, hardware - 3);

    m_threads.reserve(count);
    for (i32 i = 0; i < count; ++i)
      m_threads.emplace_back([this, i](std::stop_token st) { loop(st, i); });
  }

  inline void loop(std::stop_token st, i32 index) {
    logging::ThreadScope thread(std::format("worker-{}", index));

    while (true) {
      Job job;
      {
        std::unique_lock lock(m_mutex);
        if (!m_cv.wait(lock, st, [this] { return !m_queue.empty(); })) return;
        job = std::move(m_queue.extract(m_queue.begin()).mapped());
      }
      job();
    }
  }

  inline TaskID enqueue(i32 priority, Job job) {
    const TaskID id = (static_cast<TaskID>(priority) << 32) | static_cast<TaskID>(++m_nextSequence);
    m_queue.insert_or_assign(id, std::move(job));
    return id;
  }
};

template <typename T>
class Task {
  struct State {
    std::mutex       mutex;
    TaskID           id       = 0;
    i32              priority = 0;
    TaskStatus       status   = TaskStatus::Submitted;
    std::optional<T> result;
  };

  std::shared_ptr<State> m_state;

public:
  Task() = default;

  inline explicit Task(std::function<T()> job, i32 priority = 0)
  : m_state(std::make_shared<State>()) {
    std::lock_guard lock(m_state->mutex);

    m_state->priority = priority;
    m_state->id       = SCHEDULER.submit(priority, [state = m_state, job = std::move(job)] {
      {
        std::lock_guard lock(state->mutex);
        if (state->status != TaskStatus::Submitted) return;
        state->status = TaskStatus::InProgress;
      }

      T result = job();

      std::lock_guard lock(state->mutex);
      state->result = std::move(result);
      state->status = TaskStatus::Completed;
    });
  }

  inline ~Task() { cancel(); }

  Task(const Task&)            = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&&) noexcept = default;

  inline Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      cancel();
      m_state = std::move(other.m_state);
    }
    return *this;
  }

  inline bool valid() const { return m_state != nullptr; }

  inline TaskStatus status() const {
    if (m_state == nullptr) return TaskStatus::Cancelled;
    std::lock_guard lock(m_state->mutex);
    return m_state->status;
  }

  inline bool ready() const { return status() == TaskStatus::Completed; }

  inline std::optional<T> take() {
    if (m_state == nullptr) return std::nullopt;

    std::optional<T> result;
    {
      std::lock_guard lock(m_state->mutex);
      if (m_state->status != TaskStatus::Completed) return std::nullopt;
      result = std::move(m_state->result);
    }

    m_state.reset();
    return result;
  }

  inline void updatePriority(i32 priority) {
    if (m_state == nullptr) return;

    std::lock_guard lock(m_state->mutex);
    if (m_state->status != TaskStatus::Submitted || m_state->priority == priority) return;

    if (auto id = SCHEDULER.updatePriority(m_state->id, priority)) {
      m_state->id       = *id;
      m_state->priority = priority;
    }
  }

  inline bool cancel() {
    if (m_state == nullptr) return false;

    {
      std::lock_guard lock(m_state->mutex);
      if (m_state->status != TaskStatus::Submitted || !SCHEDULER.withdraw(m_state->id)) return false;
      m_state->status = TaskStatus::Cancelled;
    }

    m_state.reset();
    return true;
  }
};
