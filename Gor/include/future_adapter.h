#ifndef FUTURE_ADAPTER
#define FUTURE_ADAPTER

#include <coroutine>
#include <future>

template <typename... Args>
struct std::coroutine_traits<std::future<void>, Args...>
{
  struct promise_type
  {
    std::promise<void> p;
    auto get_return_object() { return p.get_future(); }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void set_exception(std::exception_ptr e) { p.set_exception(std::move(e)); }
    void unhandled_exception() { p.set_exception(std::current_exception()); }
    void return_void() { p.set_value(); }
  };
};

template <typename R, typename... Args>
struct std::coroutine_traits<std::future<R>, Args...>
{
  struct promise_type
  {
    std::promise<R> p;
    auto get_return_object() { return p.get_future(); }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void set_exception(std::exception_ptr e) { p.set_exception(std::move(e)); }
    void unhandled_exception() { p.set_exception(std::current_exception()); }
    template <typename U> void return_value(U &&u) { p.set_value(std::forward<U>(u)); }
  };
};

#endif // FUTURE_ADAPTER
