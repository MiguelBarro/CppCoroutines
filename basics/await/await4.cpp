// https://devblogs.microsoft.com/oldnewthing/20191209-00/?p=103195
// cl /EHsc /std:c++20 /Zi await4.cpp

#include <coroutine>
#include <iostream>

using namespace std;

struct resumable_thing
{
    struct promise_type;
    struct awaiter;

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

        // resumable_thing shouldn't have member variables besides the coroutine handle
        // the coroutine context keeps a unique promise in the context but the resumable_thing
        // is instantiated multiple times
        resumable_thing get_return_object()
        {
            return resumable_thing{coroutine_handle<promise_type>::from_promise(*this)};
        }

        auto initial_suspend() { return suspend_never{}; }
        auto final_suspend() noexcept { return suspend_always{}; }

        void return_void() noexcept { }

        void unhandled_exception() noexcept
        {
            std::terminate();
        }

        awaiter await_transform(std::suspend_always) { return awaiter{}; }
    };

    // ad hoc awaiter object
    struct awaiter
    {
        coroutine_handle<promise_type> _coroutine = nullptr;

        bool await_ready() const
        {
            return false;
        }

        void await_suspend(coroutine_handle<promise_type> coroutine)
        {
            // coroutine handle allows access to the promise
            _coroutine = coroutine;
        }

        int await_resume()
        {
            int & res = _coroutine.promise()._value;
            res = 42;
            return res;
        }
    };

    // non framework requirement, user custom method
    void resume()
    {
        if (_coroutine && !_coroutine.done())
            _coroutine.resume();
    }

    // non framework requirement, user custom method
    operator int() const
    {
        if (_coroutine && _coroutine.done())
            return _coroutine.promise()._value;

        throw runtime_error("not ready");
    }
};

/*
 * Is important to note that awaiters are convenience objects which launch asynchronous processes
 * to do some job and manage to resume the coroutine when the job is done providing the result.
 * Awaiters is what other languages, like python, call coroutines and should be the only piece
 * of each library coroutine framework the user should be aware of.
 * Note that the promise object await_transform method allows the coroutine library designed to hide
 * the awaiter behind 'python coroutine' like functions providing a natural syntax for the library user.
 * */
resumable_thing get_value()
{
    cout << "get_value: called" << endl;
    // int res = co_await resumable_thing::awaiter{};
    int res = co_await std::suspend_always{};
    cout << "get_value: resumed" << endl;
}

int main()
{
    cout << "main: calling get_value" << endl;
    resumable_thing value = get_value();
    cout << "main: resuming get_value" << endl;
    value.resume();
    cout << "main: value was " << value << endl;

    return 0;
}
