# Cpp的协程

## Reference

-   [一篇文章搞懂c++ 20 协程 Coroutine](https://zhuanlan.zhihu.com/p/615828280)
-   [CppCoroutines](https://github.com/bennyhuo/CppCoroutines)

## 概念

对C++20的协程，最简单的理解协程是可以重入的特殊函数。就是这个函数在执行的过程，可以（通过co_await ,或者co_yield）挂起，然后在外部（通过coroutine_handle）恢复运行。

C++的协程（协程函数）内部可以用co_await , co_yield.两个关键字挂起协程，co_return,关键字进行返回:
-   co_await: `cw_ret = co_await awaiter;` 或者 `cw_ret = co_await fun()`, 先计算表达式fun，fun返回结果，就是一个等待体awaiter。或者 `cw_ret = co_await expr`, 如果定义了 await_transform, 则 `co_await expr` 相当于 co_await promise.await_transform(expr) 了。

-   co_yield: `co_yield  cy_ret;` cy_ret会保存在promise承诺对象中（通过yield_value函数）。在协程外部可以通过promise得到。
-   co_return: `co_return cr_ret;` cr_ret会保存在promise承诺对象中（通过return_value函数）。在协程外部可以通过promise得到。要注意，cr_ret并不是协程的返回值。这个是有区别的。

协程的几个重要概念:
-   协程状态 (coroutine state)，记录协程状态，是分配于堆的内部对象：
    -   承诺对象
    -   形参（协程函数的参数）
    -   协程挂起的点
    -   临时变量
-   承诺对象（promise），从协程内部操纵。协程通过此对象提交其结果或异常。
-   协程句柄（coroutine handle），协程的唯一标示。用于恢复协程执行或销毁协程帧。
-   等待体（awaiter），co_await 关键字调用的对象。

### 协程状态（coroutine state）

协程状态（coroutine state）是协程启动开始时，new空间存放协程状态，协程状态记录协程函数的参数，协程的运行状态，变量。挂起时的断点。

注意，协程状态 (coroutine state)并不是就是协程函数的返回值RET。

#### 协程的挂起和线程的运行

协程的挂起本身不会阻塞一个线程的运行。当一个线程运行运行到协程挂起后, 线程不会阻塞而是返回到上次进入协程的地方(创建协程之处或 `coroutine_handle.resume()` 之处), 接着运行。

运行 initial_suspend() 的函数体时, 协程不是挂起状态, 

运行 co_await, co_yield, co_return 的对应接口的代码时, 比如: await_transform, 不算作挂起。协程挂起的情况有:
-   initial_suspend 返回会挂起的等待体 (e.g. suspend_always)。
-   final_suspend 返回会挂起的等待体 (e.g. suspend_always)。
-   等待体的 await_suspend 返回某些特定的值(void, true, 返回非本协程的句柄)后, 协程才是挂起状态。

### 承诺对象（promise）

承诺对象的表现形式必须是result::promise_type，result为协程函数的返回值。

承诺对象是一个实现若干接口，用于辅助协程，构造协程函数返回值；提交传递co_yield，co_return的返回值。明确协程启动阶段是否立即挂起；以及协程内部发生异常时的处理方式。其接口包括：
-   auto get_return_object() ：用于生成协程函数的返回对象。
-   auto initial_suspend()：用于明确初始化后，协程函数的执行行为，返回值为等待体（awaiter），用co_wait调用其返回值。返回值为std::suspend_always 表示协程启动后立即挂起（不执行第一行协程函数的代码），返回std::suspend_never 表示协程启动后不立即挂起。（既然是返回等待体，你可以自己在这儿选择进行什么等待操作）
-   void return_value(T v)/void return_void()：void return_value(T v) 表示协程没有返回值。用`co_return v`后会调用这个函数，可以保存co_return的结果。void return_void(): 表示协程没有返回值。用法 `co_return;`。
-   auto yield_value(T v)：调用co_yield后会调用这个函数，可以保存co_yield的结果，其返回其返回值为std::suspend_always表示协程会挂起，如果返回std::suspend_never表示不挂起。
-   auto final_suspend() noexcept：在协程退出是调用的接口，返回std::suspend_never ，自动销毁 coroutine state 对象。若 final_suspend 返回 std::suspend_always 则需要用户自行调用 handle.destroy() 进行销毁。但值得注意的是返回std::suspend_always并不会挂起协程。
-   void unhandled_exception(): 如果协程因未捕获的异常结束，则调用 `promise.unhandled_exception()`。

前面我们提到在协程创建的时候，会new协程状态（coroutine state）。你可以通过可以在 promise_type 中重载 operator new 和 operator delete，使用自己的内存分配接口。

其他接口:
-   awaiter await_transform(T v): 定义了 await_transform 函数之后，co_await expr 就相当于 co_await promise.await_transform(expr) 了。

### 协程句柄（coroutine handle）

协程句柄（coroutine handle）是一个协程的标示，用于操作协程恢复，销毁的句柄。

协程句柄的表现形式是`std::coroutine_handle<promise_type>`，其模板参数为承诺对象（promise）类型。句柄有几个重要函数：
-   resume(): 函数可以恢复协程。该函数结束后, 会保存"现场", 然后转到协程运行。当协程再次挂起时, 会还原"现场", 即转到该语句的下一条语句运行。
-   done()函数可以判断协程是否已经完成。返回false标示协程还没有完成，还在挂起。

协程句柄和承诺对象之间是可以相互转化的。

-   `std::coroutine_handle<promise_type>::from_promise`： 这是一个静态函数，可以从承诺对象（promise）得到相应句柄。
-   `std::coroutine_handle<promise_type>::promise()` 函数可以从协程句柄coroutine handle得到对应的承诺对象（promise）

### 等待体（awaiter）

co_wait 关键字会调用一个等待体对象(awaiter)。这个对象内部也有3个接口。根据接口co_wait 决定进行什么操作:
-   bool await_ready()：等待体是否准备好了，返回 false ，表示协程没有准备好，立即调用await_suspend。返回true，表示已经准备好了。
-   auto await_suspend(std::coroutine_handle<> handle): 放置协程前的操作。当然操作后有可能不挂起, 具体看返回值。如果 await_ready 返回 false, 则调用的接口。其中handle参数就是调用等待体的协程，其返回值有3种可能
    -   void 同返回true
    -   bool 返回true 立即挂起，返回false 不挂起。
    -   返回某个协程句柄（coroutine handle），立即恢复对应句柄的运行。
    -   抛出异常，此时当前协程恢复执行，并在当前协程当中抛出异常。
-   auto await_resume() ：放置协程恢复前的操作。协程挂起后恢复时，调用的接口。返回值作为co_wait 操作的返回值。

等待体（awaiter）值得用更加详细的笔墨书写一章，我们就放一下，先了解其有2个特化类型:
-   std::suspend_never类，不挂起的的特化等待体类型。表示不再挂起协程。
-   std::suspend_always类，挂起的特化等待体类型。表示继续挂起协程。

## 基本的编程结构

承诺对象: `coro_ret::promise_type`
协程句柄: `std::coroutine_handle<promise_type>`
运行顺序:
-   promise_type: 进入协程前调用。
-   get_return_object: 会构造 coro_ret 并返回。
-   initial_suspend: get_return_object 之后调用。
-   final_suspend: 协程结束后会调用。
-   ~promise_type: 协程结束后会调用。
-   ~coro_ret: 协程函数的返回对象生命周期结束后会调用。

```cpp
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>


//!coro_ret 协程函数的返回值，内部定义promise_type，承诺对象
template <typename T>
struct coro_ret
{
   struct promise_type;
   using handle_type = std::coroutine_handle<promise_type>;
   //! 协程句柄
   handle_type coro_handle_;

   coro_ret(handle_type h)
      : coro_handle_(h)
  {
  }
   coro_ret(const coro_ret&) = delete;
   coro_ret(coro_ret&& s)
      : coro_handle_(s.coro_)
  {
       s.coro_handle_ = nullptr;
  }
   ~coro_ret()
  {
       //!自行销毁
       if (coro_handle_)
           coro_handle_.destroy();
  }
   coro_ret& operator=(const coro_ret&) = delete;
   coro_ret& operator=(coro_ret&& s)
  {
       coro_handle_ = s.coro_handle_;
       s.coro_handle_ = nullptr;
       return *this;
  }

   //!恢复协程，返回是否结束
   bool move_next()
  {
       coro_handle_.resume();
       return coro_handle_.done();
  }
   //!通过promise获取数据，返回值
   T get()
  {
       return coro_handle_.promise().return_data_;
  }
   //!promise_type就是承诺对象，承诺对象用于协程内外交流
   struct promise_type
  {
       promise_type() = default;
       ~promise_type() = default;

       //!生成协程返回值
       auto get_return_object()
      {
           return coro_ret<T>{handle_type::from_promise(*this)};
      }

       //! 注意这个函数,返回的就是awaiter
       //! 如果返回std::suspend_never{}，就不挂起，
       //! 返回std::suspend_always{} 挂起
       //! 当然你也可以返回其他awaiter
       auto initial_suspend()
      {
           //return std::suspend_never{};
           return std::suspend_always{};
      }
       //!co_return 后这个函数会被调用
       void return_value(T v)
      {
           return_data_ = v;
           return;
      }
       //!
       auto yield_value(T v)
      {
           std::cout << "yield_value invoked." << std::endl;
           return_data_ = v;
           return std::suspend_always{};
      }
       //! 在协程最后退出后调用的接口。
       //! 若 final_suspend 返回 std::suspend_always 则需要用户自行调用
       //! handle.destroy() 进行销毁，但注意final_suspend被调用时协程已经结束
       //! 返回std::suspend_always并不会挂起协程（实测 VSC++ 2022）
       auto final_suspend() noexcept
      {
           std::cout << "final_suspend invoked." << std::endl;
           return std::suspend_always{};
      }
       //
       void unhandled_exception()
      {
           std::exit(1);
      }
       //返回值
       T return_data_;
  };
};


//这就是一个协程函数
coro_ret<int> coroutine_7in7out()
{
   //进入协程看initial_suspend，返回std::suspend_always{};会有一次挂起

   std::cout << "Coroutine co_await std::suspend_never" << std::endl;
   //co_await std::suspend_never{} 不会挂起
   co_await std::suspend_never{};
   std::cout << "Coroutine co_await std::suspend_always" << std::endl;
   co_await std::suspend_always{};

   std::cout << "Coroutine stage 1 ,co_yield" << std::endl;
   co_yield 101;
   std::cout << "Coroutine stage 2 ,co_yield" << std::endl;
   co_yield 202;
   std::cout << "Coroutine stage 3 ,co_yield" << std::endl;
   co_yield 303;
   std::cout << "Coroutine stage end, co_return" << std::endl;
   co_return 808;
}

int main(int argc, char* argv[])
{
   bool done = false;
   std::cout << "Start coroutine_7in7out ()\n";
   //调用协程,得到返回值c_r，后面使用这个返回值来管理协程。
   auto c_r = coroutine_7in7out();
   //第一次停止因为initial_suspend 返回的是suspend_always
   //此时没有进入Stage 1
   std::cout << "Coroutine " << (done ? "is done " : "isn't done ")
       << "ret =" << c_r.get() << std::endl;
   done = c_r.move_next();
   //此时是，co_await std::suspend_always{}
   std::cout << "Coroutine " << (done ? "is done " : "isn't done ")
       << "ret =" << c_r.get() << std::endl;
   done = c_r.move_next();
   //此时打印Stage 1
   std::cout << "Coroutine " << (done ? "is done " : "isn't done ")
       << "ret =" << c_r.get() << std::endl;
   done = c_r.move_next();
   std::cout << "Coroutine " << (done ? "is done " : "isn't done ")
       << "ret =" << c_r.get() << std::endl;
   done = c_r.move_next();
   std::cout << "Coroutine " << (done ? "is done " : "isn't done ")
       << "ret =" << c_r.get() << std::endl;
   done = c_r.move_next();
   std::cout << "Coroutine " << (done ? "is done " : "isn't done ")
       << "ret =" << c_r.get() << std::endl;
   return 0;
}
```

## 协程库

-   [cppcoro](https://github.com/lewissbaker/cppcoro)
