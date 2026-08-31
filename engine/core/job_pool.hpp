#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <thread>
#include <vector>

class JobPool {
public:
  using Job = std::function<void()>;

private:
  std::mutex                  m_mutex;
  std::condition_variable_any m_cv;
  std::deque<Job>             m_jobs;
  std::vector<std::jthread>   m_threads;

public:
  JobPool();
  ~JobPool() = default;

  JobPool(const JobPool&) = delete;
  JobPool(JobPool&&) = delete;
  JobPool& operator=(const JobPool&) = delete;
  JobPool& operator=(JobPool&&) = delete;

  void submit(Job job);

private:
  void loop(std::stop_token st);
};