#pragma once

#include <functional>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "io_utils.h"

class DelayedExecutor {
public:
  explicit DelayedExecutor(std::function<void()>&& func, long long delay) : m_func(std::move(func)) {
    using namespace std;
    using namespace std::chrono;

    auto now = system_clock::now();
    auto current = duration_cast<milliseconds>(now.time_since_epoch()).count();
    m_scheduled_time = current + delay;
  }

  void operator()() {
    m_func();
  }

  long long delay() const {
    using namespace std;
    using namespace std::chrono;

    auto now = system_clock::now();
    auto current = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return current - m_scheduled_time;
  }

  long long get_scheduled_time() const {
    return m_scheduled_time;
  }

private:
  std::function<void()> m_func;
  long long m_scheduled_time;
};

class DelayedExecutorCompare {
public:
  bool operator()(const DelayedExecutor& a, const DelayedExecutor& b) {
    return a.get_scheduled_time() > b.get_scheduled_time();
  }
};

class Scheduler {
public:
  Scheduler() {
    m_is_active.store(true, std::memory_order_relaxed);
    m_work_thread = std::thread(&Scheduler::run_loop, this);
  }

  ~Scheduler() {
    m_is_active.store(false, std::memory_order_relaxed);
    shutdown(false);
    join();
  }

  void run_loop() {
    while (m_is_active.load(std::memory_order_relaxed) || !m_executor_queue.empty()) {
      std::unique_lock lock(m_queue_mutex);
      if (m_executor_queue.empty()) {
        m_queue_cv.wait(lock);
        if (m_executor_queue.empty()) {
          continue;
        }
      }

      auto executor = m_executor_queue.top();
      auto delay = executor.delay();
      if (delay > 0) {
        auto status = m_queue_cv.wait_for(lock, std::chrono::milliseconds(delay));
        if (status != std::cv_status::timeout) {
          // a new executable should be executed before.
          debug("status != std::cv_status::timeout");
          continue;
        }
      }
      m_executor_queue.pop();
      lock.unlock();

      executor();
    }
  }

  void execute(std::function<void()>&& func, long long delay) {
    delay = delay < 0 ? 0 : delay;

    if (m_is_active.load(std::memory_order_relaxed)) {
      std::unique_lock lock(m_queue_mutex);
      bool need_to_notify = m_executor_queue.empty() || delay < m_executor_queue.top().delay();
      m_executor_queue.push(DelayedExecutor(std::move(func), delay));
      lock.unlock();
      if (need_to_notify) {
        m_queue_cv.notify_one();
      }
    }
  }

  void shutdown(bool does_wait_for_complete=true) {
    m_is_active.store(false, std::memory_order_relaxed);
    if (does_wait_for_complete) {
      std::lock_guard lg(m_queue_mutex);
      decltype(m_executor_queue) empty_queue;
      std::swap(m_executor_queue, empty_queue);
    }
    m_queue_cv.notify_all();
  }

  void join() {
    if (m_work_thread.joinable()) {
      m_work_thread.join();
    }
  }
private:
  std::mutex m_queue_mutex;
  std::condition_variable m_queue_cv;

  std::priority_queue<DelayedExecutor, std::vector<DelayedExecutor>, DelayedExecutorCompare> m_executor_queue;
  
  std::atomic<bool> m_is_active;
  std::thread m_work_thread;
};
