# Cpp Coroutine

## 协程的基本概念

C++20 引入的协程是一个无栈协程框架，它不是一个可以直接使用的具体函数，而是一套允许用户自定义协程行为的底层语言特性。

首先要纠正一个常见误解：std::coroutine 或 co_await 并不是一个像 std::thread 那样的具体对象。

协程是一种可以暂停和恢复执行的函数，它允许在不需要多线程的情况下实现异步编程。C++20 引入了协程支持，通过三个关键字 `co_await`、`co_yield`、`co_return` 来标识协程函数。

有四个概念:
-   协程函数
-   协程函数的返回值
-   Promise
-   Awaiter

## 协程函数

包含 `co_await`、`co_yield`、`co_return` 这三个关键字中的任何一个，这个函数就被标记为一个协程函数。编译器会自动将这样的函数转换为状态机。

## 协程函数的返回值

协程函数的返回值类型必须包含一个嵌套的 `promise_type` 类型，该类型负责管理协程的状态和生命周期。

## Promise

Promise 是协程的核心控制结构，定义了协程的行为。其接口包括：

- `get_return_object()`: 创建并返回协程的返回值对象
- `initial_suspend()`: 决定协程开始时是否立即挂起
- `final_suspend()`: 决定协程结束时是否挂起
- `yield_value(value)`: 处理 `co_yield` 表达式
- `return_value(value)`: 处理 `co_return` 表达式（有返回值）
- `return_void()`: 处理 `co_return` 表达式（无返回值）
- `unhandled_exception()`: 处理协程中未捕获的异常
- `await_transform`: 它允许 Promise 类型对协程函数体内每一个 co_await 表达式进行拦截和转换。

## Awaiter

Awaiter 控制 `co_await` 表达式的行为，其接口包括：

- `await_ready()`: 返回是否不需要挂起
- `await_suspend(handle)`: 挂起时执行的操作
- `await_resume()`: 恢复时返回的值

## 协程程序的结构

```cpp
template<typename T, typename Executor>
class Task {
public:
  using promise_type = TaskPromise<T, Executor>;
  using handle_type = std::coroutine_handle<promise_type>;

private:
  handle_type handle_;  // 用于控制协程生命周期的句柄
};

template<typename T, typename Executor>
class TaskPromise {
public:
    TaskPromise() = default;        // 调用时机：协程开始前，在协程帧中构造promise对象时
    ~TaskPromise() = default;       // 调用时机：协程帧销毁时

    // 调用时机：协程开始前，在构造完promise对象后立即调用
    // 用途：创建并返回给调用者的Task对象
    Task<T, Executor> get_return_object() {
        return Task<T, Executor>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
    }
    
    // 调用时机：协程开始执行函数体之前
    // 返回suspend_always：协程创建后立即挂起，需要手动resume
    // 返回suspend_never：协程创建后立即开始执行
    std::suspend_always initial_suspend() noexcept { return {}; }
    
    // 调用时机：协程执行完毕（co_return或异常）后，销毁局部变量之前
    // 返回suspend_always：协程结束后保持挂起状态，可以手动获取结果
    // 返回suspend_never：协程结束后自动销毁
    std::suspend_always final_suspend() noexcept { return {}; }
    
    // 调用时机：遇到co_yield表达式时
    // 用途：处理yield的值，通常保存到promise中供外部获取
    void yield_value(T value) { result_ = std::move(value); }
    
    // 调用时机：遇到co_return表达式时（有返回值版本）
    // 用途：保存协程的最终返回值
    void return_value(T value) { result_ = std::move(value); }
    
    // 调用时机：协程中发生未捕获的异常时
    // 用途：异常处理，通常保存异常指针供后续重新抛出
    void unhandled_exception() { exception_ = std::current_exception(); }
    
    // 自定义方法：获取协程结果（非标准接口）
    // 调用时机：由用户代码在适当的时候调用
    T get_result() { 
        if (exception_) std::rethrow_exception(exception_);
        return result_;
    }

private:
    T result_;                   // 存储协程的返回值或yield值
    std::exception_ptr exception_; // 存储协程中的异常
};

template<typename T>
class Awaiter {
public:
    // 调用时机：co_await表达式求值时立即调用
    // 返回true：操作已完成，不需要挂起，直接调用await_resume()
    // 返回false：操作未完成，需要挂起，继续调用await_suspend()
    bool await_ready() const noexcept { 
        return false; // 通常需要挂起
    }
    
    // 调用时机：await_ready()返回false后，协程挂起之前
    // 参数handle：当前协程的句柄，可用于后续恢复协程
    // 用途：安排异步操作，操作完成后通过handle.resume()恢复协程
    void await_suspend(std::coroutine_handle<> handle) {
        // 安排协程在适当的时候恢复
        // 例如：将handle存储起来，等异步操作完成时调用handle.resume()
    }
    
    // 调用时机：协程从挂起状态恢复时
    // 返回值：作为co_await表达式的结果
    T await_resume() {
        return value_;
    }
    
private:
    T value_;  // 存储异步操作的结果
};

Task<int> coroutine_func() {
    // co_await调用流程：
    // 1. 构造SomeAwaiter对象
    // 2. 调用awaiter.await_ready()
    // 3. 如果返回false，调用awaiter.await_suspend()并挂起
    // 4. 恢复时调用awaiter.await_resume()，结果赋值给value
    int value = co_await some_async_operation();
    
    // co_yield调用流程：
    // 1. 调用promise.yield_value(value * 2)
    // 2. 挂起协程，控制权返回给调用者
    co_yield value * 2;
    
    // co_return调用流程：
    // 1. 调用promise.return_value(value + 10)
    // 2. 调用promise.final_suspend()
    // 3. 根据final_suspend()决定是否挂起
    co_return value + 10;
}
```

## 完整的协程执行流程：

1. **协程创建阶段**：
   - 分配协程帧内存 (协程帧在堆上分配)
   - 构造promise对象（调用构造函数）
   - 调用`get_return_object()`创建返回给调用者的Task
   - 调用`initial_suspend()`决定是否立即挂起

2. **协程执行阶段**：
   - 遇到`co_await`：调用awaiter的三部曲（ready→suspend→resume）
   - 遇到`co_yield`：调用`promise.yield_value()`并挂起
   - 遇到`co_return`：调用`promise.return_value()`或`return_void()`

3. **协程结束阶段**：
   - 调用`final_suspend()`决定最终状态
   - 如果返回suspend类型，协程保持挂起，可手动销毁 (Task对象析构时，协程帧不会自动销毁！)
   - 如果返回non-suspend类型，协程自动销毁

4. **异常处理**：
   - 发生异常时调用`unhandled_exception()`
   - 异常信息保存在promise中供后续处理

## 常用工具类型

- `std::suspend_always`: 总是挂起
- `std::suspend_never`: 从不挂起
- `std::coroutine_handle<>`: 协程句柄，用于控制协程
- `std::coroutine_traits`: 用于推断协程的 promise_type

## `await_transform`

它允许 Promise 类型对协程函数体内每一个 co_await 表达式进行拦截和转换。

[![](https://mermaid.ink/img/pako:eNptkd9uEkEYxV9lMleaAIGFLbsbU9NCrZdceCXTNFsY_iTsLhmWtEpINFoLVmibtBVba00btIlpFhONWKB9GXZ2ueIV3JmNNDbezWTO-Z3zfVODGSOLoQJzJWM9U1CJCZ4kkb6QRtBu7TgX2_abTXvrymn3QMZYVdfVognwRpkguAKCwXmwWEOQnr-gp90UMbRiBTvfB_an7dSDNTJPO5a9-4V-bALuWzWJqldyBtEAbezae52HCNaRjvRFjwSYjAETLLl34loHbu-Vs38BUopyx36PFbjPEmxrQJvX9rvfk82Wc_R6stVyRyOvGtITHJb02vkYlvj21E9M8ryOxSVLXt4_kwH37Bs9vqGts3H_ioXMXsv-hKH_1uGpnOzNzMmP0s7w0LX2JvsfXMuaDo_4TprX437LbvRmpe8uZ-V2Je8_c9CyV9E5_kHb3fHohq2E16QnXbf9y945BAsMgAnDT4cNenlu9_v050vv86bDJu-F9CVOepx2Bl-dwaWvYeGz4XyDp132lTAA86SYhYpJqjgANUw0lV1hDekAIGgWsIYRVLxjFufUaslEEOl1z1ZW9aeGof11EqOaL0Alp5Yq3q1azqomThbVPFFvJVjPYpIwqroJFTHKEVCpwQ2oRMNyKCLFRTkiikI8GosF4DOoBKWQIMcjsiTIoihHhTmhHoDPeWg4JMXk6Fw4HpaliCDJklT_A3KWOhY?type=png)](https://mermaid.live/edit#pako:eNptkd9uEkEYxV9lMleaAIGFLbsbU9NCrZdceCXTNFsY_iTsLhmWtEpINFoLVmibtBVba00btIlpFhONWKB9GXZ2ueIV3JmNNDbezWTO-Z3zfVODGSOLoQJzJWM9U1CJCZ4kkb6QRtBu7TgX2_abTXvrymn3QMZYVdfVognwRpkguAKCwXmwWEOQnr-gp90UMbRiBTvfB_an7dSDNTJPO5a9-4V-bALuWzWJqldyBtEAbezae52HCNaRjvRFjwSYjAETLLl34loHbu-Vs38BUopyx36PFbjPEmxrQJvX9rvfk82Wc_R6stVyRyOvGtITHJb02vkYlvj21E9M8ryOxSVLXt4_kwH37Bs9vqGts3H_ioXMXsv-hKH_1uGpnOzNzMmP0s7w0LX2JvsfXMuaDo_4TprX437LbvRmpe8uZ-V2Je8_c9CyV9E5_kHb3fHohq2E16QnXbf9y945BAsMgAnDT4cNenlu9_v050vv86bDJu-F9CVOepx2Bl-dwaWvYeGz4XyDp132lTAA86SYhYpJqjgANUw0lV1hDekAIGgWsIYRVLxjFufUaslEEOl1z1ZW9aeGof11EqOaL0Alp5Yq3q1azqomThbVPFFvJVjPYpIwqroJFTHKEVCpwQ2oRMNyKCLFRTkiikI8GosF4DOoBKWQIMcjsiTIoihHhTmhHoDPeWg4JMXk6Fw4HpaliCDJklT_A3KWOhY)

## 示例：简单的生成器

```cpp
template<typename T>
class Generator {
public:
    struct promise_type {
        T value_;
        
        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        std::suspend_always yield_value(T value) {
            value_ = value;
            return {};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    // Generator 类的其他实现...
};
```
