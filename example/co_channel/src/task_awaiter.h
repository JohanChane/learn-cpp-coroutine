#pragma once

#include <coroutine>

#include "task.h"
#include "executor.h"

template<typename T, typename Executor>
class Task;

template<typename T, typename Executor>
class TaskAwaiter {
public:
  TaskAwaiter(Task<T, Executor>&& t, AbstractExecutor* e) : m_task(std::move(t)), m_executor(e) {}
  TaskAwaiter(TaskAwaiter&& ta) : m_task(std::exchange(ta.m_task, {})) {}     // 虽然没有定义 task 的默认函数构造, 但是可以用的。

  constexpr bool await_ready() const noexcept {
    return false;
  }
  
  void await_suspend(std::coroutine_handle<> handle) {
    m_task.finally([handle, this]() {
      m_executor->execute([handle]() {
        handle.resume();
        });
      });
  }

  T await_resume() {
    return m_task.get_result();
  }
  
private:
  Task<T, Executor> m_task;
  AbstractExecutor* m_executor;
};
