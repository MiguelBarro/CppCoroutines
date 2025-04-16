// cl /EHsc /std:c++20 /Zi await2.cpp
#include <coroutine>
#include <iostream>

using namespace std;

struct resumable_thing
{
    struct promise_type;

    coroutine_handle<promise_type> _coroutine = nullptr;

    explicit resumable_thing(coroutine_handle<promise_type> coroutine)
        : _coroutine(coroutine)
    {
    }

    ~resumable_thing()
    {
        if (_coroutine)
            _coroutine.destroy();
    }

    resumable_thing() = default;
    resumable_thing(const resumable_thing&) = delete;
    resumable_thing& operator=(const resumable_thing&) = delete;

    resumable_thing(resumable_thing&& rhs) noexcept
        : _coroutine(rhs._coroutine)
    {
        rhs._coroutine = nullptr;
    }

    resumable_thing& operator=(resumable_thing&& other) noexcept
    {
        if (&other != this)
        {
            _coroutine = other._coroutine;
            other._coroutine = nullptr;
        }
    }

    void resume()
    {
        _coroutine.resume();
    }

    struct promise_type
    {
        resumable_thing get_return_object()
        {
            return resumable_thing{coroutine_handle<promise_type>::from_promise(*this)};
        }

        auto initial_suspend() { return suspend_never{}; }
        auto final_suspend() noexcept { return suspend_never{}; }

        void return_void() {}

        void unhandled_exception() noexcept
        {
            std::terminate();
        }

    };
};

resumable_thing named_counter(string name)
{
    cout << "counter: (" << name << ") was called" << endl;
    for (unsigned i = 1; ; ++i)
    {
        co_await suspend_always{};
        cout << "counter (" << name << ") resumed (#" << i << ")" << endl;
    }
}

int main()
{
    resumable_thing counter_a = named_counter("a");
    resumable_thing counter_b = named_counter("b");
    counter_a.resume();
    counter_b.resume();
    counter_b.resume();
    counter_a.resume();
}
