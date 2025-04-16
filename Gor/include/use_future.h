#ifndef CORO_USE_FUTURE
# define CORO_USE_FUTURE

#include <future>

#include <asio/async_result.hpp>

struct use_future_t {};
constexpr use_future_t use_future;

template <>
class asio::async_result<use_future_t>
{
    std::future<void>
};



#endif
