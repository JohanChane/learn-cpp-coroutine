#pragma once

#include <coroutine>

#include "executor.h"
#include "scheduler.h"

class SleepAwaiter {
public:
  explicit SleepAwaiter(AbstractExecutor* executor, long long duration) : m_executor(executor), m_duration(duration) {}

  bool await_ready() {
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    static auto scheduler = Scheduler();
    scheduler.execute([this, handle]() {
        m_executor->execute([handle]() {
            handle.resume();
          });
      }, m_duration);
  }

  void await_resume() {}

private:
  AbstractExecutor* m_executor;
  long long m_duration;
};
