#include <iostream>
#include <thread>

#include "task.h"
#include "executor.h"
#include "io_utils.h"

using namespace std;

Task<int, AsyncExecutor> sub_simple_task1() {
  using namespace std::chrono_literals;
  co_await 3s;
  co_return 1;
}

Task<int, NewThreadExecutor> sub_simple_task2() {
  using namespace std::chrono_literals;
  co_await 1s;
  co_return 2;
}

Task<int, LooperExecutor> simple_task() {
  auto result1 = co_await sub_simple_task1();
  auto result2 = co_await sub_simple_task2();
  co_return 1 + result1 + result2;
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

void test_scheduler() {
  auto scheduler = Scheduler();
  
  scheduler.execute([]() {
      debug("5");
    }, 2500);
  scheduler.execute([]() {
      debug("3");
    }, 1500);
  scheduler.execute([]() {
      debug("6");
    }, 3000);
  scheduler.execute([]() {
      debug("2");
    }, 1000);
  scheduler.execute([]() {
      debug("1");
    }, 500);
  scheduler.execute([]() {
      debug("4");
    }, 2000);

  scheduler.shutdown(false);
  scheduler.join();
}

int main() {
  test_task();
  test_scheduler();

  return 0;
}
