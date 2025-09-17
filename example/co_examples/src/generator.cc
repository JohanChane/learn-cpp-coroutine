#include <coroutine>
#include <exception>
#include <iostream>

using namespace std;

struct Generator {
  class ExhaustedException : exception {};

  struct promise_type {
    Generator get_return_object() {
      return Generator{coroutine_handle<promise_type>::from_promise(*this)};
    }

    suspend_always initial_suspend() {
      return {};
    }

    suspend_always final_suspend() noexcept {
      return {};
    }

    suspend_always await_transform(int value) {
      this->value = value;
      is_ready = true;
      return {};
    }

    void unhandled_exception() {
    }
    void return_void() {
    }

    int value;
    bool is_ready;
  };

  Generator(coroutine_handle<promise_type> handle ) : handle(handle) {
  }
  ~Generator() {
    handle.destroy();
  }

  bool has_next() {
    if (handle.done()) {
      return false;
    }

    if (!handle.promise().is_ready) {
      handle.resume();
    }

    if (handle.done()) {
      return false;
    } else {
      return true;
    }
  }

  int next() {
    if (has_next()) {
      handle.promise().is_ready = false;
      return handle.promise().value;
    }

    throw ExhaustedException();
  }

  coroutine_handle<promise_type> handle;
};

Generator sequence() {
  int i = 0;
  while (i < 5) {
    co_await i++;
  }
}

int main() {
  auto g = sequence();
  for (int i = 0; i < 15; i++) {
    if (g.has_next()) {
      cout << g.next() << endl;
    }
  }

  return 0;
}
