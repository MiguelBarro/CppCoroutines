// > cl /EHsc /std:c++20 /Zi yield1.cpp

#include <coroutine>
#include <iostream>

class generator
{
public:

    struct promise_type;

private:

    using handle = std::coroutine_handle<promise_type>;

    generator(handle h) : coro_(h) {}
    handle coro_;

public:

    struct promise_type
    {
        int current_value_;
        std::exception_ptr exception_;

        static generator get_return_object_on_allocation_failure()
        {
            return {nullptr};
        }

        generator get_return_object()
        {
            return {handle::from_promise(*this)};
        }

        void unhandled_exception()
        {
            exception_ = std::current_exception();
        }

        std::suspend_always initial_suspend()
        {
            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        // this method will be called for each co_yield expression that returns an int
        std::suspend_always yield_value(int value)
        {
            current_value_ = value;
            return {};
        }

        void return_void() {}
    };

    bool move_next()
    {
        return coro_ ? (coro_.resume(), !coro_.done()) : false;
    }

    int current_value()
    {
        return coro_.promise().current_value_;
    }

    ~generator()
    {
        if (coro_)
            coro_.destroy();
    }
};

generator f()
{
    co_yield 1;
    co_yield 2;
}

int main()
{
    auto g = f();
    while (g.move_next())
        std::cout << g.current_value() << std::endl;
}
