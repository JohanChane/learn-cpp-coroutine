#pragma once

#include <coroutine>

#include "task.h"
#include "executor.h"

template<typename T, typename Executor>
class Task;

template<typename T, typename Executor>
class TaskAwaiter {
public:
  TaskAwaiter(Task<T, Executor>&& t, AbstractExecutor* e) : task(std::move(t)), executor(e) {}
  TaskAwaiter(TaskAwaiter&& ta) : task(std::exchange(ta.task, {})) {}     // 虽然没有定义 task 的默认函数构造, 但是可以用的。

  constexpr bool await_ready() const noexcept {
    return false;
  }
  
  void await_suspend(std::coroutine_handle<> handle) {
    task.finally([handle, this]() {
      executor->execute([handle]() {
        handle.resume();
        });
      });
  }

  T await_resume() {
    return task.get_result();
  }
  
private:
  Task<T, Executor> task;
  AbstractExecutor* executor;
};
