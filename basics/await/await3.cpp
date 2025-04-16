// https://devblogs.microsoft.com/oldnewthing/20191209-00/?p=103195
// cl /EHsc /std:c++20 /Zi await3.cpp

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
        {
            cout << "destroying coroutine which is " << ios::boolalpha << _coroutine.done() << endl;
            _coroutine.destroy();
        }
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

    struct promise_type
    {
        int _value = 0;

        resumable_thing get_return_object()
        {
            return resumable_thing{coroutine_handle<promise_type>::from_promise(*this)};
        }

        auto initial_suspend() { return suspend_never{}; }
        // We must suspend_always because otherwise the promise object will be destroyed before
        // value retrieval. Keeping the returned value outside this object (as the shared states
        // in std::promise) would allow to suspend_never here.
        auto final_suspend() noexcept { return suspend_always{}; }

        void return_value(const int& res) noexcept
        {
            _value = res;
        }

        void unhandled_exception() noexcept
        {
            std::terminate();
        }
    };

    // non framework requirement, user custom method
    int get()
    {
        return _coroutine.promise()._value;
    }

    // non framework requirement, user custom method
    void resume()
    {
        if (_coroutine && !_coroutine.done())
            _coroutine.resume();
    }
};

resumable_thing get_value()
{
    cout << "get_value: called" << endl;
    co_await suspend_always{};
    cout << "get_value: resumed" << endl;
    co_return 30;
}

int main()
{
    cout << "main: calling get_value" << endl;
    resumable_thing value = get_value();
    cout << "main: resuming get_value" << endl;
    value.resume();
    cout << "main: value was " << value.get() << endl;

    return 0;
}
