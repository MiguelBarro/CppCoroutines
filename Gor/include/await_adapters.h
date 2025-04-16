#ifndef AWAIT_ADAPTERS
#define AWAIT_ADAPTERS

#include <algorithm>
#include <coroutine>
#include <optional>

#include <asio.hpp>

#include <handler_allocator.h>

template <typename AsyncStream, typename BufferSequence>
auto async_write(AsyncStream& s, BufferSequence const& buffers)
{
    struct [[nodiscard]] Awaiter
    {
        AsyncStream& s;
        BufferSequence const& buffers;
        handler_allocator alloc;
        size_t n;
        std::error_code ec;

        Awaiter(AsyncStream& sp, BufferSequence const& bp)
            : s(sp)
            , buffers(bp) {}

        bool await_ready() { return false; }

        size_t await_resume()
        {
            if (ec)
            {
                std::cerr << "Error in async_write: " << ec.message() << std::endl;
                throw std::system_error(ec);
            }
            return n;
        }

        void await_suspend(std::coroutine_handle<> coro)
        {
            async_write(s, buffers,
                    make_custom_alloc_handler(alloc,
                        [this, coro](auto ec, auto n) mutable
                        {
                            this->n = n;
                            this->ec = ec;
                            coro.resume();
                        }));
        }
    };

    return Awaiter{s, buffers};
}

template <typename AsyncStream, typename BufferSequence>
auto async_read_some(AsyncStream& s, BufferSequence const& buffers)
{
    struct [[nodiscard]] Awaiter
    {
        AsyncStream& s;
        BufferSequence const& buffers;
        handler_allocator alloc;
        size_t n;
        std::error_code ec;

        Awaiter(AsyncStream& sp, BufferSequence const& bp)
            : s(sp)
            , buffers(bp) {}

        bool await_ready() { return false; }

        size_t await_resume()
        {
            if (ec)
            {
                if (ec != asio::error::eof)
                    std::cerr << "Error in async_read_some: " << ec.message() << std::endl;
                throw std::system_error(ec);
            }
            return n;
        }

        void await_suspend(std::coroutine_handle<> coro)
        {
            s.async_read_some(buffers,
                    make_custom_alloc_handler(alloc,
                        [this, coro](auto ec, auto n) mutable
                        {
                            this->n = n;
                            this->ec = ec;
                            coro.resume();
                        }));
        }
    };

    return Awaiter{s, buffers};
}

template <typename AcceptorSocket, typename AsyncStream>
auto async_accept(AcceptorSocket& a, AsyncStream& s)
{
    struct [[nodiscard]] Awaiter
    {
        AcceptorSocket& a;
        AsyncStream& s;
        std::error_code ec;

        Awaiter(AcceptorSocket& ap, AsyncStream& sp)
            : a(ap)
            , s(sp) {}

        bool await_ready() { return false; }

        auto await_resume()
        {
            if (ec)
                throw std::system_error(ec);
        }

        void await_suspend(std::coroutine_handle<> coro)
        {
            a.async_accept(s, [this, coro](auto ec) mutable
                    {
                        this->ec = ec;
                        coro.resume();
                    });
        }
    };

    return Awaiter{a, s};
}

template <typename Clock, typename R, typename P>
auto async_wait(
        asio::basic_waitable_timer<Clock> &t,
        std::chrono::duration<R, P> d)
{
    struct [[nodiscard]] Awaiter
    {
        asio::basic_waitable_timer<Clock> &t;
        std::chrono::duration<R, P> d;
        std::error_code ec {};

        bool await_ready() { return d.count() == 0; }

        void await_resume()
        {
            if (ec)
                throw std::system_error(ec);
        }

        void await_suspend(std::coroutine_handle<> coro)
        {
            t.expires_from_now(d);
            t.async_wait([this, coro](auto ec) mutable {this->ec = ec; coro.resume();});
        }
    };

    return Awaiter{ t, d };
}

template <typename socket_type, typename endpoint_iterator_type>
auto async_connect(socket_type& socket, endpoint_iterator_type& peer_endpoint)
{
    struct [[nodiscard]] Awaiter
    {
        socket_type& socket_;
        endpoint_iterator_type& peer_endpoint_;
        std::error_code ec_ {};

        bool await_ready() { return false; }

        void await_resume()
        {
            if (ec_)
                throw std::system_error(ec_);
        }

        void await_suspend(std::coroutine_handle<> coro)
        {
            asio::async_connect(socket_, peer_endpoint_,
                    [this, coro](auto ec, const endpoint_iterator_type&) mutable
                    {
                        ec_ = ec;
                        coro.resume();
                    });
        }
    };

    return Awaiter{socket, peer_endpoint};
}

template <typename IOService>
auto post(IOService& io)
{
    struct [[nodiscard]] Awaiter
    {
        IOService& io_;

        bool await_ready() { return false; }

        void await_resume() {}

        void await_suspend(std::coroutine_handle<> coro)
        {
            io_.post([coro]() mutable
                    {
                        coro.resume();
                    });
        }
    };

    return Awaiter{ io };
}

#endif // AWAIT_ADAPTERS
