#pragma once
#ifndef THREADPOOL_WINRT_H
#define THREADPOOL_WINRT_H

#include <coroutine>
#include <cstdint>

#include <windows.h>

#include <winrt/Windows.Foundation.h>

namespace threadpool_winrt
{
    // Pool ancillary wrapper
    class environment
    {
        environment(environment const &) = delete;
        environment & operator=(environment const &) = delete;

        TP_CALLBACK_ENVIRON env_;
        PTP_POOL pool_ = nullptr;
        PTP_CLEANUP_GROUP cleanup_ = nullptr;

        void destroy()
        {
            if (cleanup_)
            {
                CloseThreadpoolCleanupGroupMembers(cleanup_, FALSE, nullptr);
                CloseThreadpoolCleanupGroup(cleanup_);
            }
            if (pool_)
            {
                CloseThreadpool(pool_);
            }
            DestroyThreadpoolEnvironment(&env_);
        }

        public:

        environment(std::uint32_t threads) try
        {
            InitializeThreadpoolEnvironment(&env_);

            pool_ = CreateThreadpool(nullptr);
            if ( nullptr == pool_)
                winrt::throw_last_error();

            SetThreadpoolThreadMaximum(pool_, threads);
            if (FALSE == SetThreadpoolThreadMinimum(pool_, threads))
                winrt::throw_last_error();

            cleanup_ = CreateThreadpoolCleanupGroup();
            if ( nullptr == cleanup_)
                winrt::throw_last_error();

            SetThreadpoolCallbackPool(&env_, pool_);
            SetThreadpoolCallbackCleanupGroup(&env_, cleanup_, nullptr);

        } catch (...)
        {
            destroy();
            throw;
        }

        ~environment() noexcept
        {
            destroy();
        }

        PTP_CALLBACK_ENVIRON get() noexcept
        {
            return &env_;
        }
    };

    // metadata utilities
    template <class T>
    constexpr bool is_async_v = false;

    template <>
    constexpr bool is_async_v<winrt::Windows::Foundation::IAsyncAction> = true;

    template <typename TResult>
    constexpr bool is_async_v<winrt::Windows::Foundation::IAsyncOperation<TResult>> = true;

    template <typename TProgress>
    constexpr bool is_async_v<winrt::Windows::Foundation::IAsyncActionWithProgress<TProgress>> = true;

    template <typename TResult, typename TProgress>
    constexpr bool is_async_v<winrt::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>> = true;

    // resume always on the provided thread pool
    template <typename Async>
    struct pool_awaiter : winrt::impl::await_adapter<Async, false>
    {
        using await_base = winrt::impl::await_adapter<Async, false>;
        using await_base::await_base;

        // thread pool callback to resume the coroutine
        static VOID CALLBACK threadpool_callback(PTP_CALLBACK_INSTANCE, PVOID context) noexcept
        {
            std::coroutine_handle<>::from_address(context)();
        };

        template <typename T>
        bool await_suspend(std::coroutine_handle<T> handle)
        {
            // Associate cancellation token
            this->set_cancellable_promise_from_handle(handle);

            // Delegate into the thread pool
            this->async.Completed([this, handle](
                    const auto&,
                    const winrt::Windows::Foundation::AsyncStatus& asyncStatus){
                // Update
                this->status = asyncStatus;

                // If async.Completed() tries to resume, due to cancellation or
                // already completed, then don't suspend, that is, instead
                // of running into the thread pool run in the co_await thread
                if (!this->suspending.exchange(false, std::memory_order_release)
                    && !TrySubmitThreadpoolCallback(
                            &pool_awaiter::threadpool_callback,
                            handle.address(),
                            handle.promise().tp_env.get()))
                {
                    winrt::throw_last_error();
                }
            });

            // Decide to suspend or not based on underlying async status
            return this->suspending.exchange(false, std::memory_order_acquire);
        }
    };

    // inherit from the framework's promise type
    template <typename Derived, typename Async>
    struct pool_promise : winrt::impl::promise_base<Derived, Async>
    {
        threadpool_winrt::environment& tp_env;

        using promise_base = winrt::impl::promise_base<Derived, Async>;

        template <typename... Args>
        pool_promise(threadpool_winrt::environment& env, Args&&...)
            : tp_env(env)
        {
        }

        // Use SFINAE to only enable for WinRT non-async types
        template<typename T,
                 std::enable_if_t<!threadpool_winrt::is_async_v<std::decay_t<T>>, bool> = true>
        auto await_transform(T&& exp)
        {
            return promise_base::await_transform(exp);
        }

        // Use SFINAE to only enable for WinRT async types
        template<typename T,
                 std::enable_if_t<threadpool_winrt::is_async_v<std::decay_t<T>>, bool> = true>
        pool_awaiter<T> await_transform(T&& async)
        {
            return {promise_base::await_transform(async)};
        }

        // Cancellation token support
        auto await_transform(winrt::get_cancellation_token_t t) noexcept
        {
            return promise_base::await_transform(t);
        }
    };

} // namespace threadpool_winrt

namespace std
{
    template <typename... Args>
    struct coroutine_traits<winrt::Windows::Foundation::IAsyncAction, threadpool_winrt::environment&, Args...>
    {
        struct promise_type final
            : threadpool_winrt::pool_promise<promise_type, winrt::Windows::Foundation::IAsyncAction>
        {
            using base_type = threadpool_winrt::pool_promise<promise_type, winrt::Windows::Foundation::IAsyncAction>;
            using base_type::base_type;

            void return_void() const noexcept {}
        };
    };

    template <typename TResult, typename... Args>
    struct coroutine_traits<winrt::Windows::Foundation::IAsyncOperation<TResult>, threadpool_winrt::environment&, Args...>
    {
        struct promise_type final
            : threadpool_winrt::pool_promise<promise_type, winrt::Windows::Foundation::IAsyncOperation<TResult>>
        {
            using base_type = threadpool_winrt::pool_promise<promise_type, winrt::Windows::Foundation::IAsyncOperation<TResult>>;
            using base_type::base_type;

            TResult get_return_value() noexcept
            {
                return std::move(m_result);
            }

            TResult copy_return_value() noexcept
            {
                return m_result;
            }

            void return_value(TResult&& value) noexcept
            {
                m_result = std::move(value);
            }

            void return_value(TResult const& value) noexcept
            {
                m_result = value;
            }

            TResult m_result{ winrt::impl::empty_value<TResult>() };
         };
    };

} // namespace std

#endif // THREADPOOL_WINRT_H
