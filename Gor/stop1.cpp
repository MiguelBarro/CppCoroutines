#include <coroutine>
#include <iostream>
#include <system_error>

#include <asio.hpp>
#include <asio/system_timer.hpp>

#include <await_adapters.h>
#include <future_adapter.h>

using namespace std;
using namespace std::chrono;
using namespace asio;

template <typename Clock, typename R, typename P>
auto better_async_wait(basic_waitable_timer<Clock> &t, duration<R, P> d)
{
  struct [[nodiscard]] Awaiter
  {
    basic_waitable_timer<Clock> &t;
    duration<R, P> d;
    error_code ec;

    bool await_ready() { return d.count() == 0; }

    void await_resume()
    {
      if (ec)
        throw std::system_error(ec);
    }

    struct Callback
    {
      Awaiter *me;
      coroutine_handle<> coro;

      void operator() (std::error_code ec)
      {
        me->ec = ec;
        auto tmp = coro;
        coro = {};
        tmp.resume();
      }

      Callback(Awaiter *me, std::coroutine_handle<> coro)
          : me(me)
          , coro(coro)
      {}

      Callback(Callback const& other)
          : me(other.me)
          , coro(other.coro)
      {
      }

      Callback(Callback && other)
          : me(other.me)
          , coro(other.coro)
      {
        other.coro = nullptr;
      }

      ~Callback()
      {
        if (coro)
        {
          me->ec = error::operation_aborted;
          coro.resume();
        }
      }
    };

    void await_suspend(std::coroutine_handle<> coro)
    {
      t.expires_from_now(d);
      t.async_wait(Callback{this, coro});
    }
  };

  return Awaiter{ t, d, {} };
}


std::future<void> noisy_clock(system_timer &timer) try
{
    for (;;)
    {
        co_await better_async_wait(timer, 1s);
        cout << "tick" << endl;
        co_await better_async_wait(timer, 1s);
        cout << "tock" << endl;
    }
}
catch (std::exception const& e)
{
    cout << "caught: " << e.what() << endl;
}

int main()
{
  {
    io_service io;
    system_timer timer(io);
    auto f = noisy_clock(timer);
    system_timer fast_timer(io, 1s);
    fast_timer.async_wait([&](auto){io.stop();});
    io.run();
    cout << "done" << endl;
  }
  cout << "io_service destroyed" << endl;
}
