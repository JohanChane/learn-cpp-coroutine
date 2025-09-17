#pragma once

#include <coroutine>

#include "executor.h"

class DispatchAwaiter {
public:
  explicit DispatchAwaiter(AbstractExecutor* e) : m_executor(e) {}
  DispatchAwaiter(const DispatchAwaiter&) = delete;
  DispatchAwaiter& operator=(const DispatchAwaiter&) = delete;

  bool await_ready() {
    return false;
  }
  void await_suspend(std::coroutine_handle<> handle) {
    m_executor->execute([handle]() {
        handle.resume();      // 如果某个线程执行 handle.resume(), 则由该线程运行 handle 对应的线程。
      });
  }
  void await_resume() {}
private:
  AbstractExecutor* m_executor;
};
