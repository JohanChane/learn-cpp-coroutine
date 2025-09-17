#pragma once

#include <list>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "channel_awaiter.h"

template<typename T>
class WriterAwaiter;

template<typename T>
class ReaderAwaiter;

template<typename T>
class Channel {
public:
  struct ChannelException : public std::exception {
    const char* what() const noexcept {
      return "Channel is closed.";
    }
  };

  Channel(unsigned long capacity = 0) : m_buffer_capacity(capacity) {
    m_is_active.store(true, std::memory_order_relaxed);
  }
  Channel(const Channel&) = delete;
  Channel(Channel&&) = delete;
  Channel& operator=(const Channel&) = delete;
  ~Channel() {
    close();
  }

  void check_closed() {
    if (!m_is_active.load(std::memory_order_relaxed)) {
      throw ChannelException{};
    }
  }

  ReaderAwaiter<T> read() {
    check_closed();
    std::lock_guard lg(m_mtx);
    return {this};
  }
  WriterAwaiter<T> write(T& value) {
    check_closed();
    std::lock_guard lg(m_mtx);
    return {this, value};
  }
  ReaderAwaiter<T> operator>>(T& value) {
    auto ra = read();
    ra.m_value_ptr = &value;
    return ra;
  }
  WriterAwaiter<T> operator<<(T& value) {
    return write(value);
  }

  void try_push_reader(ReaderAwaiter<T>* reader) {
    check_closed();
    std::unique_lock lock(m_mtx);

    if (!m_buffer.empty()) {
      auto v = m_buffer.front();
      m_buffer.pop();
      lock.unlock();

      reader->resume(v);
      return;
    }

    if (m_writer_list.size()) {
      auto writer = m_writer_list.front();
      m_writer_list.pop_front();
      lock.unlock();

      reader->resume(writer->m_value);
      writer->resume();

      return;
    }

    m_reader_list.push_back(reader);
    lock.unlock();
  }

  void try_push_writer(WriterAwaiter<T>* writer) {
    check_closed();
    std::unique_lock lock(m_mtx);

    if (m_reader_list.size()) {
      auto reader = m_reader_list.front();
      m_reader_list.pop_front();
      lock.unlock();

      reader->resume(writer->m_value);
      writer->resume();

      return;
    }

    if (m_buffer.size() < m_buffer_capacity) {
      m_buffer.push(writer->m_value);
      lock.unlock();

      writer->resume();

      return;
    }

    m_writer_list.push_back(writer);
    lock.unlock();
  }

  void remove_writer(WriterAwaiter<T>* wa) {
    m_writer_list.remove(wa);
  }
  void remove_reader(ReaderAwaiter<T>* ra) {
    m_reader_list.remove(ra);
  }

  void close() {
    bool expect = true;
    if(m_is_active.compare_exchange_strong(expect, false, std::memory_order_relaxed)) {
      clean_up();
    }
  }
  void clean_up() {
    std::lock_guard lg(m_mtx);
    for (auto& w : m_writer_list) {
      w->resume();
    }
    m_writer_list.clear();

    for (auto& r : m_reader_list) {
      r->resume();
    }
    m_reader_list.clear();
    
    decltype(m_buffer) empty_buffer;
    std::exchange(m_buffer, empty_buffer);
  }

  bool is_active() {
    return m_is_active.load(std::memory_order_relaxed);
  }

private:
  std::mutex m_mtx;
  std::condition_variable m_cv;

  std::queue<T> m_buffer;
  unsigned long m_buffer_capacity;
  std::list<ReaderAwaiter<T>*> m_reader_list;
  std::list<WriterAwaiter<T>*> m_writer_list;     // 可以只存储指针的原因: 只在 read 之后才调用 writer 的 resume。这里的 list 是存入多个 producer 的 writer 的。

  std::atomic<bool> m_is_active;
};
