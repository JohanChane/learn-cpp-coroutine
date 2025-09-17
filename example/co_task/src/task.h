#pragma once

#include <coroutine>
#include <utility>

#include "task_promise.h"
#include "task_awaiter.h"

template<typename T, typename Executor>
class Task {
public:
  using promise_type = TaskPromise<T, Executor>;
  using handle_type = std::coroutine_handle<promise_type>;
  
  explicit Task(handle_type h) : m_co_handle(h) {}
  Task(const Task&) = delete;
  Task(Task&& task) : m_co_handle(std::exchange(task.m_co_handle, {})) {}
  Task& operator=(const Task&) = delete;
  ~Task() {
    if (m_co_handle) {
      m_co_handle.destroy();
    }
  }

  T get_result() {
    return m_co_handle.promise().get_result();
  }

  Task& then(std::function<void(T)>&& func) {
    m_co_handle.promise().on_completed([func](auto result) {
        try {
          func(result.get());
        } catch (std::exception& e) {
          // ignore
        }
      });
    return *this;
  }

  Task& catching(std::function<void(const std::exception&)>&& func) {
    m_co_handle.promise().on_completed([func](auto result) {
        try {
          result.get();
        } catch (const std::exception& e) {
          func(e);
        }
      });
    return *this;
  }

  Task& finally(std::function<void()>&& func) {
    m_co_handle.promise().on_completed([func](auto) {
        func();
      });
    return *this;
  }

private:
  handle_type m_co_handle;
};
