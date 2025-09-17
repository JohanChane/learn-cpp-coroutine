#pragma once

#include <functional>
#include <future>
#include <queue>
#include <mutex>

struct AbstractExecutor {
  virtual void execute(std::function<void()>&& func) = 0;
};

struct NoopExecutor : public AbstractExecutor {
  void execute(std::function<void()>&& func) override {
    func();
  }
};

struct AsyncExecutor : public AbstractExecutor {
  void execute(std::function<void()>&& func) override {
    auto future = std::async(std::move(func));
  }
};

struct NewThreadExecutor : public AbstractExecutor {
  void execute(std::function<void()>&& func) override {
    std::thread(std::move(func)).detach();
  }
};

class LooperExecutor : public AbstractExecutor {
public:
  LooperExecutor() {
    m_is_active.store(true, std::memory_order_relaxed);
    m_work_thread = std::thread(&LooperExecutor::run_loop, this);
  }
  ~LooperExecutor() {
    shutdown(false);
    if (m_work_thread.joinable()) {
      m_work_thread.join();
    }
  }

  void run_loop() {
    // `!executor_queue.empty()` 为了 does_wait_for_complete = true.
    while (m_is_active.load(std::memory_order_relaxed) || !m_executor_queue.empty()) {
      std::unique_lock lock(m_queue_mutex);
      if (m_executor_queue.empty()) {
        m_queue_cv.wait(lock);
        if (m_executor_queue.empty()) {
          continue;
        }
      }
      auto func = m_executor_queue.front();
      m_executor_queue.pop();
      lock.unlock();

      func();
    }
  }

  void execute(std::function<void()>&& func) override {
    if (m_is_active.load(std::memory_order_relaxed)) {
      std::unique_lock lock(m_queue_mutex);
      m_executor_queue.push(std::move(func));
      lock.unlock();
      m_queue_cv.notify_one();
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
private:
  std::queue<std::function<void()>> m_executor_queue;
  std::mutex m_queue_mutex;
  std::condition_variable m_queue_cv;

  std::atomic<bool> m_is_active;    // 用于 run_loop 的自动退出和 shutdown
  std::thread m_work_thread;
};
