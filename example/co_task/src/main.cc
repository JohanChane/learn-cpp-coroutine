#include <iostream>
#include <thread>

#include "task.h"
#include "executor.h"
#include "io_utils.h"

using namespace std;

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
  auto result1 = co_await sub_simple_task1();
  auto result2 = co_await sub_simple_task2();
  this_thread::sleep_for(500ms);
  co_return 1 + result1 + result2;
}

void test_task() {
  auto co_ret = simple_task();
  
  // ## 回调的方式取得结果 (调用 f 的时机是不确定的, 只是确定在有结果之后)
  co_ret.then([](int i) {
      debug("result: ", i);
    }).catching([](const std::exception& e) {
      cout << "exception: " << e.what() << endl;
    });

  debug("after then");

  // ## 同步阻塞的方式取得结果
  try {
    auto result = co_ret.get_result();
    debug("result: ", result);
  } catch (const exception& e) {
    cout << e.what() << endl;
  }
}

int main() {
  test_task();

  return 0;
}
