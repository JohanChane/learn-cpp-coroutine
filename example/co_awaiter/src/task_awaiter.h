#pragma once

#include <coroutine>

#include "task.h"
#include "executor.h"
#include "common_awaiter.h"

template<typename T, typename Executor>
class Task;

template<typename T, typename Executor>
class TaskAwaiter : public Awaiter<T> {
public:
  TaskAwaiter(Task<T, Executor>&& t) : m_task(std::move(t)) {}
  TaskAwaiter(TaskAwaiter&& ta) : Awaiter<T>(ta), m_task(std::move(ta.m_task)) {}

protected:
  void suspend_helper() override {
    m_task.finally([this]() {
        this->resume_unsafe();
      });
  }
  void resume_helper() override {
    this->m_result = Result<T>(m_task.get_result());
  }
  
private:
  Task<T, Executor> m_task;
};
