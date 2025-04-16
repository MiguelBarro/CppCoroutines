// cl /EHsc /std:c++20 /Zi await1.cpp

#include <coroutine>
#include <iostream>
#include <version>

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

        return *this;
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

resumable_thing counter()
{
    cout << "counter: called" << endl;
    for (unsigned i = 1; ; ++i)
    {
        co_await suspend_always{};
        cout << "counter: resumed (#" << i << ")" << endl;
    }
}

int main()
{
    cout << "__cpp_impl_coroutine: " << __cpp_impl_coroutine << endl;
    cout << "main: calling counter" << endl;
    resumable_thing the_counter = counter();
    cout << "main: resuming counter" << endl;
    the_counter.resume();
    the_counter.resume();
    cout << "main: done" << endl;
}
