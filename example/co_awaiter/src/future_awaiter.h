#pragma once

#include <future>

#include "common_awaiter.h"

template<typename T>
class FutureAwaiter : public Awaiter<T> {
public:
  explicit FutureAwaiter(std::future<T>&& future) : m_future(std::move(future)) {}
  FutureAwaiter(const FutureAwaiter&) = delete;
  //FutureAwaiter(FutureAwaiter&& awaiter) : Awaiter<T>(std::move(awaiter)), m_future(std::move(awaiter.m_future)) {}
  FutureAwaiter(FutureAwaiter&& awaiter) = default;
  FutureAwaiter& operator=(const FutureAwaiter&) = delete;
  
protected:
  void suspend_helper() override {
    std::thread([this]() {
        this->resume(this->m_future.get());
      }).detach();
  }
  
private:
  std::future<T> m_future;
};
