#include <iostream>
#include <thread>

#include "task.h"
#include "executor.h"
#include "io_utils.h"
#include "channel.h"

using namespace std;
using namespace std::chrono_literals;

Task<void, LooperExecutor> producer(Channel<int>& channel) {
  try {
    for (int i = 0; i < 10; i++) {
      debug("producer send: ", i);
      co_await (channel << i);
      //co_await 500ms;
      co_await 1s;
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
  test_channel();

  return 0;
}
