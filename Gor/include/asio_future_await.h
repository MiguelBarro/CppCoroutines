#ifndef ASIO_FUTURE_AWAIT
#define ASIO_FUTURE_AWAIT

#include <coroutine>
#include <memory>

#include <asio/system_timer.hpp>
#include <asio/use_future.hpp>

// My allocator
class pre_cpp20_allocator
    : public std::allocator<void>
{
public:
    // Inherit constructors
    using allocator<void>::allocator;

    // reintroduce the C++20 removed rebind mechanism for asio's sake
    template <class Other>
    struct rebind {
        using other = std::allocator<Other>;
    };
};

#if defined(ASIO_HAS_CONSTEXPR)
constexpr asio::use_future_t<pre_cpp20_allocator> my_use_future;
#elif defined(ASIO_MSVC)
__declspec(selectany) asio::use_future_t<pre_cpp20_allocator> my_use_future;
#endif

// My awaiter
template <typename T>
class asio_future_awaiter
{
    asio::io_service& io_;
    std::future<T> f_; // keep future alive
    asio::system_timer t_;
    std::coroutine_handle<> coro_ = {};
    constexpr static auto peek_period = std::chrono::milliseconds(100);

    void resume_or_wait(const std::error_code& ec)
    {
        if (ec)
            return;

        if (!await_ready())
        {
            // cout << "keep waiting..." << endl;
            // keep waiting
            t_.expires_from_now(peek_period);
            t_.async_wait([this](const std::error_code& ec){resume_or_wait(ec);});
        }
        else
        {   // we are done, resume execution
            coro_();
        }
    }

public:

    asio_future_awaiter(asio::io_service& io, std::future<T>&& f)
        : io_(io)
        , f_(std::move(f))
        , t_(io_)
    {}

    bool await_ready() const
    {
        return std::future_status::ready == f_.wait_for(std::chrono::seconds::zero());
    }

    void await_suspend(std::coroutine_handle<> coro)
    {
        // keep coroutine alive
        coro_ = coro;
        // start the timer
        resume_or_wait(std::error_code());
    }

    decltype(auto) await_resume()
    {
        coro_ = {}; // release coroutine
        return f_.get();
    }
};

// My async functions relying on my awaiter
template <typename Alloc, typename Socket, typename MutableBufferSequence>
asio_future_awaiter<size_t>
async_read_some(Socket& s, const MutableBufferSequence& buffers, asio::use_future_t<Alloc> uf)
{
    return asio_future_awaiter<size_t>{s.get_io_service(), s.async_read_some(buffers, uf)};
}

template <typename Alloc, typename Socket, typename MutableBufferSequence>
asio_future_awaiter<size_t>
async_write(Socket& s, const MutableBufferSequence& buffers, asio::use_future_t<Alloc> uf)
{
    return asio_future_awaiter<size_t>{s.get_io_service(), asio::async_write(s, buffers, uf)};
}

template <typename Alloc, typename Service>
asio_future_awaiter<void>
post(Service& s, asio::use_future_t<Alloc> uf)
{
    return asio_future_awaiter<void>{s, s.post(uf)};
}

#endif // ASIO_FUTURE_AWAIT
