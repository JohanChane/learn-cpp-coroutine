#pragma once

#include <exception>

template<typename T>
class Result {
public:
  explicit Result(T&& v) : m_value(std::move(v)) {}
  explicit Result(std::exception_ptr exc_ptr) : m_exc_ptr(exc_ptr) {}

  T get() {
    if (!m_exc_ptr) {
      return m_value;
    } else {
      std::rethrow_exception(m_exc_ptr);
    }
  }

private:
  T m_value;
  std::exception_ptr m_exc_ptr;
};

template<>
class Result<void> {
public:
  explicit Result() {}
  explicit Result(std::exception_ptr exc_ptr) : m_exc_ptr(exc_ptr) {}

  void get() {
    if (m_exc_ptr) {
      std::rethrow_exception(m_exc_ptr);
    }
  }

private:
  std::exception_ptr m_exc_ptr;
};
