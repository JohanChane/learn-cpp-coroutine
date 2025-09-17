#include <iostream>
#include <thread>

#include "task.h"
#include "executor.h"
#include "io_utils.h"
#include "channel.h"
#include "future_awaiter.h"

using namespace std;
using namespace std::chrono_literals;

Task<int, AsyncExecutor> sub_simple_task1() {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(3s);
  co_return 1;
}

Task<int, NewThreadExecutor> sub_simple_task2() {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
  co_return 2;
}

Task<int, LooperExecutor> simple_task() {
  auto result1 = co_await sub_simple_task1().as_awaiter();
  auto result2 = co_await sub_simple_task2();
  auto result3 = co_await FutureAwaiter(std::async([]() {
      std::this_thread::sleep_for(1s);
      return 3;
      }));
  co_return result1 + result2 + result3 + 4;
}

void test_task() {
  auto co_ret = simple_task();
  
  // ## 回调的方式取得结果
  co_ret.then([](int i) {
      debug("result: ", i);
    }).catching([](const std::exception& e) {
      cout << "exception: " << e.what() << endl;
    });

  // ## 同步阻塞的方式取得结果
  try {
    auto result = co_ret.get_result();
    debug("result: ", result);
  } catch (const exception& e) {
    cout << e.what() << endl;
  }
}
Task<void, LooperExecutor> producer(Channel<int>& channel) {
  try {
    for (int i = 0; i < 10; i++) {
      debug("producer send: ", i);
      co_await (channel << i);
      co_await 500ms;
    }
  } catch (std::exception& e) {
    debug("exception: ", e.what());
  }

  co_await 3s;
  channel.close();
  debug("channel closed");
}

Task<void, LooperExecutor> consumer1(Channel<int>& channel) {
  int v;
  while (channel.is_active()) {
    try {
      co_await (channel >> v);
      debug("consumer1 received: ", v);
      co_await 500ms;
    } catch (std::exception& e) {
      debug("exception: ", e.what());
    }
  }
}

Task<void, LooperExecutor> consumer2(Channel<int>& channel) {
  while (channel.is_active()) {
    try {
      auto received = co_await channel.read();
      debug("consumer2 received: ", received);
      co_await 300ms;
    } catch (std::exception& e) {
      debug("exception: ", e.what());
    }
  }
}

void test_channel() {
  auto channel = Channel<int>(5);
  auto c1 = consumer1(channel);
  auto c2 = consumer2(channel);
  std::this_thread::sleep_for(2s);
  auto p = producer(channel);

  std::this_thread::sleep_for(5s);
  debug("test_channel end");
}

int main() {
  test_task();
  test_channel();

  return 0;
}
