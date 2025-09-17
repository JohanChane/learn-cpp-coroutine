#pragma once

#include <coroutine>

#include "executor.h"
#include "scheduler.h"
#include "common_awaiter.h"

class SleepAwaiter : public Awaiter<void> {
public:
  explicit SleepAwaiter(long long duration) : m_duration(duration) {}
  template<typename Rep, typename Period>
  explicit SleepAwaiter(std::chrono::duration<Rep, Period>&& duration) : m_duration(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) {}

protected:
  void suspend_helper() override {
    static auto scheduler = Scheduler();
    scheduler.execute([this]() {
        this->resume();
      }, m_duration);
  }

private:
  long long m_duration;
};
