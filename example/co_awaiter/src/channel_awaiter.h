#pragma once

#include <coroutine>
#include <utility>

#include "executor.h"
#include "channel.h"
#include "common_awaiter.h"

template<typename T>
class Channel;

template<typename T>
struct WriterAwaiter : public Awaiter<void> {
  WriterAwaiter(Channel<T>* channel, T& value) : m_channel(channel), m_value(value) {}
  WriterAwaiter(WriterAwaiter&& wa)
    : Awaiter<void>(wa),
      m_channel(std::exchange(wa.m_channel, nullptr)),
      m_value(wa.m_value) {}
  ~WriterAwaiter() {
    if (m_channel) {
      m_channel->remove_writer(this);
    }
  }

  void suspend_helper() override {
    m_channel->try_push_writer(this);
  }

  void resume_helper() override {
    m_channel->check_closed();
    m_channel = nullptr;
  }

  Channel<T>* m_channel;
  T m_value;
};

template<typename T>
struct ReaderAwaiter : public Awaiter<T> {
  ReaderAwaiter(Channel<T>* channel) : m_channel(channel) {}
  ReaderAwaiter(ReaderAwaiter&& ra)
    : Awaiter<T>(ra),
      m_channel(std::exchange(ra.m_channel, nullptr)),
      m_value_ptr(std::exchange(ra.m_value_ptr, nullptr)) {}
  ~ReaderAwaiter() {
    if (m_channel) {
      m_channel->remove_reader(this);
    }
  }

  void suspend_helper() override {
    m_channel->try_push_reader(this);
  }
  void resume_helper() override {
    m_channel->check_closed();
    if (m_value_ptr) {
      *m_value_ptr = this->m_result->get();
    }
    m_channel = nullptr;
  }

  Channel<T>* m_channel;
  T* m_value_ptr;
};
