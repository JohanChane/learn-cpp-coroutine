#pragma once

#include <coroutine>
#include <utility>

#include "executor.h"
#include "channel.h"

template<typename T>
class Channel;

template<typename T>
struct WriterAwaiter {
  WriterAwaiter(Channel<T>* channel, T& value) : m_channel(channel), m_value(value) {}
  WriterAwaiter(WriterAwaiter&& wa)
    : m_channel(std::exchange(wa.m_channel, nullptr)),
      m_executor(std::exchange(wa.m_executor, nullptr)),
      m_handle(wa.m_handle),
      m_value(wa.m_value) {}
  ~WriterAwaiter() {
    if (m_channel) {
      m_channel->remove_writer(this);
    }
  }
  bool await_ready() {
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    this->m_handle = handle;
    m_channel->try_push_writer(this);
  }

  void await_resume() {
    m_channel->check_closed();
    m_channel = nullptr;
  }

  void resume() {
    if (m_executor) {
      m_executor->execute([this](){
          m_handle.resume();
        });
    } else {
      m_handle.resume();
    }
  }

  Channel<T>* m_channel;
  AbstractExecutor* m_executor = nullptr;
  std::coroutine_handle<> m_handle;
  T m_value;
};

template<typename T>
struct ReaderAwaiter {
  ReaderAwaiter(Channel<T>* channel) : m_channel(channel), m_executor(nullptr), m_value_ptr(nullptr) {}
  ReaderAwaiter(ReaderAwaiter&& ra)
    : m_channel(std::exchange(ra.m_channel, nullptr)),
      m_executor(std::exchange(ra.m_executor, nullptr)),
      m_handle(ra.m_handle), 
      m_value(ra.m_value),
      m_value_ptr(std::exchange(ra.m_value_ptr, nullptr)) {}
  ~ReaderAwaiter() {
    if (m_channel) {
      m_channel->remove_reader(this);
    }
  }

  bool await_ready() {
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    this->m_handle = handle;
    m_channel->try_push_reader(this);
  }

  int await_resume() {
    m_channel->check_closed();
    m_channel = nullptr;
    return m_value;
  }

  void resume(T& v) {
    m_value = v;
    if (m_value_ptr) {
      *m_value_ptr = v;
    }
    resume();
  }

  void resume() {
    if (m_executor) {
      m_executor->execute([this](){
          m_handle.resume();
        });
    } else {
      m_handle.resume();
    }
  }

  Channel<T>* m_channel;
  AbstractExecutor* m_executor;
  std::coroutine_handle<> m_handle;
  T m_value;
  T* m_value_ptr;
};
