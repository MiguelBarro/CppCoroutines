// cl /EHsc /std:c++20 /Zi yield2.cpp
#include <coroutine>
#include <cstddef>
#include <iostream>

using namespace std;

struct int_generator
{
    struct promise_type;
    struct iterator;

    explicit int_generator(coroutine_handle<promise_type> coroutine)
        : _coroutine(coroutine)
    {
    }

    ~int_generator()
    {
        if (_coroutine)
        {
            cout << "destroying coroutine which is " << ios::boolalpha << _coroutine.done() << endl;
            _coroutine.destroy();
        }
    }

    int_generator() = default;
    int_generator(const int_generator&) = delete;
    int_generator& operator=(const int_generator&) = delete;

    int_generator(int_generator&& rhs) noexcept
        : _coroutine(rhs._coroutine)
    {
        rhs._coroutine = nullptr;
    }

    int_generator& operator=(int_generator&& other) noexcept
    {
        if (&other != this)
        {
            _coroutine = other._coroutine;
            other._coroutine = nullptr;
        }

        return *this;
    }

    struct promise_type
    {
        int _value = 0;

        int_generator get_return_object()
        {
            return int_generator{coroutine_handle<promise_type>::from_promise(*this)};
        }

        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() noexcept { return suspend_always{}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }

        auto yield_value(int value)
        {
            _value = value;
            return suspend_always{};
        }
    };

    struct iterator // : std::iterator<input_iterator_tag, int>
    {
        using iterator_category = input_iterator_tag;
        using value_type        = int;
        using difference_type   = ptrdiff_t;
        using pointer           = const int*;
        using reference         = const int&;

        coroutine_handle<promise_type> _coroutine = nullptr;

        explicit iterator(coroutine_handle<promise_type>& h) : _coroutine(h) {}
        explicit iterator(nullptr_t) {}
        bool operator==(iterator other) const
        {
            return _coroutine == other._coroutine;
        }
        bool operator!=(iterator other) const { return !(*this == other); }

        iterator& operator++()
        {
            if (_coroutine)
            {
                _coroutine.resume();

                if (_coroutine.done())
                    _coroutine = nullptr;
            }
            return *this;
        }

        int operator*() const
        {
            return _coroutine.promise()._value;
        }
    };

    iterator begin()
    {
        if (_coroutine && !_coroutine.done())
            _coroutine.resume();
        return iterator{_coroutine};
    }

    iterator end()
    {
        return iterator{nullptr};
    }

    coroutine_handle<promise_type> _coroutine = nullptr;
};

int_generator integers(int start, int end)
{
    for (int i = start; i <= end; ++i)
        co_yield i;
}

int main()
{
    for (int i : integers(0, 9))
        cout << "(" << i << ") ";
    cout << endl;
}
