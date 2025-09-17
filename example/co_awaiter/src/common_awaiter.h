#pragma once

#include <coroutine>
#include <functional>
#include <optional>
#include <exception>

#include "executor.h"
#include "result.h"

template<typename T>
class Awaiter {
public:
  bool await_ready() {
    return false;
  }
  void await_suspend(std::coroutine_handle<> handle) {
    m_handle = handle;
    suspend_helper();
  }
  T await_resume() {
    resume_helper();
    return m_result->get();
  }

  void install_executor(AbstractExecutor* executor) {
    m_executor = executor;
  }

  void resume(T value) {
    dispatch([this, value]() {
      m_result = Result<T>(static_cast<T>(value));
      m_handle.resume();
      });
  }
  void resume_unsafe() {
    dispatch([this]() {
      m_handle.resume();
      });
  }
  void resume_exception(std::exception_ptr&& e) {
    dispatch([this, e]() {
      m_result = Result<T>(static_cast<T>(e));
      m_handle.resume();
      });
  }

protected:
  virtual void resume_helper() {}
  virtual void suspend_helper() {}
  std::optional<Result<T>> m_result;
private:
  void dispatch(std::function<void()>&& func) {
    if (m_executor) {
      m_executor->execute(std::move(func));
    } else {
      func();
    }
  }
  AbstractExecutor* m_executor = nullptr;
  std::coroutine_handle<> m_handle = nullptr;
};

template<>
class Awaiter<void> {
public:
  bool await_ready() {
    return false;
  }
  void await_suspend(std::coroutine_handle<> handle) {
    m_handle = handle;
    suspend_helper();
  }
  void await_resume() {
    resume_helper();
    m_result->get();
  }

  void install_executor(AbstractExecutor* executor) {
    m_executor = executor;
  }

  void resume() {
    dispatch([this]() {
      m_result = Result<void>();
      m_handle.resume();
      });
  }
  void resume_exception(std::exception_ptr&& e) {
    dispatch([this, e]() {
      m_result = Result<void>(e);
      m_handle.resume();
      });
  }

protected:
  virtual void resume_helper() {}
  virtual void suspend_helper() {}
  std::optional<Result<void>> m_result;
private:
  void dispatch(std::function<void()>&& func) {
    if (m_executor) {
      m_executor->execute(std::move(func));
    } else {
      func();
    }
  }
  AbstractExecutor* m_executor = nullptr;
  std::coroutine_handle<> m_handle = nullptr;
};
