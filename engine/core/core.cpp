#include "./job_pool.hpp"
#include "./random.hpp"
#include <mutex>
#include <random>

JobPool::JobPool() {
  u32 maxThreads = std::thread::hardware_concurrency() - 2u;
  m_threads.resize(maxThreads);
  for (auto& thread : m_threads)
    thread = std::jthread([this](std::stop_token st){ loop(st); });
}

void JobPool::submit(Job job) {
{
  std::lock_guard lock(m_mutex);
  m_jobs.push_back(std::move(job));
}
  m_cv.notify_one();
}

void JobPool::loop(std::stop_token st) {
  logging::register_thread(logging::Thread::worker);
  while (!st.stop_requested()) {
    std::unique_lock lock(m_mutex);

    m_cv.wait(lock, st, [this]{ return !m_jobs.empty(); });
    if (st.stop_requested()) return;
    if (m_jobs.empty()) return;

    Job job = std::move(m_jobs.front());
    m_jobs.pop_front();

    lock.unlock();

    job();
  }
}

i32 Random::i(i32 min, i32 max) {
  std::lock_guard lock(m_mutex);
  return std::uniform_int_distribution<i32>(min, max)(m_gen);
}

f32 Random::f(f32 min, f32 max) {
  std::lock_guard lock(m_mutex);
  return std::uniform_real_distribution<f32>(min, max)(m_gen);
}