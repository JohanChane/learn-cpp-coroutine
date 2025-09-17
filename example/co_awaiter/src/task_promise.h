#pragma once

#include <coroutine>
#include <optional>
#include <functional>
#include <list>
#include <chrono>

#include "task.h"
#include "result.h"
#include "task_awaiter.h"
#include "dispatch_awaiter.h"
#include "sleep_awaiter.h"
#include "channel_awaiter.h"

template<typename T, typename Executor>
class Task;

template<typename T, typename Executor>
class TaskPromise {
public:
  Task<T, Executor> get_return_object() {
    return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
  }
  DispatchAwaiter initial_suspend() {
    return DispatchAwaiter{&m_executor};
  }
  std::suspend_always final_suspend() noexcept {
    return {};
  }
  void unhandled_exception() {
    std::lock_guard lg(m_mtx);
    m_result = Result<T>(std::current_exception());

    m_cv.notify_all();
    notify_callbacks();
  }

  void return_value(T value) {
    std::lock_guard lg(m_mtx);
    m_result = Result<T>(std::move(value));
    m_cv.notify_all();
    notify_callbacks();
  }

  template<typename AwaiterImpl>
  AwaiterImpl await_transform(AwaiterImpl&& awaiter) {
    awaiter.install_executor(&m_executor);
    return awaiter;
  }

  template<typename T1, typename Executor1>
  TaskAwaiter<T1, Executor1> await_transform(Task<T1, Executor1>&& task) {
    return await_transform(TaskAwaiter<T1, Executor1>{std::move(task)});
  }

  template<typename Rep, typename Period>
  SleepAwaiter await_transform(std::chrono::duration<Rep, Period>&& duration) {
    return await_transform(SleepAwaiter(std::move(duration)));
  }

  T get_result() {
    std::unique_lock lock(m_mtx);
    if (!m_result.has_value()) {
      m_cv.wait(lock);
    }
    return m_result->get();
  }

  void on_completed(std::function<void(Result<T>)>&& func) {
    std::unique_lock lock(m_mtx);
    if (m_result.has_value()) {
      auto value = m_result.value();
      lock.unlock();
      func(value);
    } else {
      m_completion_callbacks.push_back(std::move(func));
    }
  }

private:
  void notify_callbacks() {
    auto value = m_result.value();
    for (auto& callback : m_completion_callbacks) {
      callback(value);
    }
    m_completion_callbacks.clear();
  }

private:
  std::mutex m_mtx;
  std::condition_variable m_cv;

  std::optional<Result<T>> m_result;
  std::list<std::function<void(Result<T>)>> m_completion_callbacks;

  Executor m_executor;
};

template<typename Executor>
class TaskPromise<void, Executor> {
public:
  Task<void, Executor> get_return_object() {
    return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
  }
  DispatchAwaiter initial_suspend() {
    return DispatchAwaiter{&m_executor};
  }
  std::suspend_always final_suspend() noexcept {
    return {};
  }
  void unhandled_exception() {
    std::lock_guard lg(m_mtx);
    m_result = Result<void>(std::current_exception());

    m_cv.notify_all();
    notify_callbacks();
  }

  void return_void() {
    std::lock_guard lg(m_mtx);

    m_cv.notify_all();
    notify_callbacks();
  }

  template<typename AwaiterImpl>
  AwaiterImpl await_transform(AwaiterImpl&& awaiter) {
    awaiter.install_executor(&m_executor);
    return awaiter;
  }

  template<typename T1, typename Executor1>
  TaskAwaiter<T1, Executor1> await_transform(Task<T1, Executor1>&& task) {
    return await_transform(std::move(task));
  }

  template<typename Rep, typename Period>
  SleepAwaiter await_transform(std::chrono::duration<Rep, Period>&& duration) {
    return await_transform(SleepAwaiter(std::move(duration)));
  }

  template<typename T1>
  void get_result() {
    std::unique_lock lock(m_mtx);
    if (!m_result.has_value()) {
      m_cv.wait(lock);
    }
    m_result->get();
  }

  void on_completed(std::function<void(Result<void>)>&& func) {
    std::unique_lock lock(m_mtx);
    if (m_result.has_value()) {
      auto value = m_result.value();
      lock.unlock();
      func(value);
    } else {
      m_completion_callbacks.push_back(std::move(func));
    }
  }

private:
  void notify_callbacks() {
    auto value = m_result.value();
    for (auto& callback : m_completion_callbacks) {
      callback(value);
    }
    m_completion_callbacks.clear();
  }

private:
  std::mutex m_mtx;
  std::condition_variable m_cv;

  std::optional<Result<void>> m_result;
  std::list<std::function<void(Result<void>)>> m_completion_callbacks;

  Executor m_executor;
};
