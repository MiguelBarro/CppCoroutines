// https://github.com/ChuanqiXu9/stdmodules/blob/0d5589cac865d92ae0858600e798b8a537703461/examples/Generator/Generator.cpp#L75
// > cl /EHsc /std:c++20 /Zi .\yield3.cpp

#include <coroutine>
#include <cstddef>
#include <iostream>
#include <numeric>

using namespace std;

template <typename T>
struct generator
{
    struct promise_type; // this type must be public for coroutine_traits to work it out

    private:

    coroutine_handle<promise_type> _handle;

    generator(generator const&) = delete;
    generator(generator&& rhs)
        : _handle(rhs._handle)
    {
        rhs._handle = nullptr;
    }
    explicit generator(coroutine_handle<promise_type> handle) : _handle(handle) {}

    public:

    ~generator()
    {
        if (_handle)
            _handle.destroy();
    }

    struct promise_type
    {
        T _current_value;

        suspend_always yield_value(T value)
        {
            _current_value = value;
            return {};
        }

        suspend_always initial_suspend() { return {}; }
        suspend_always final_suspend() noexcept { return {}; }
        generator get_return_object()
        {
            return generator{coroutine_handle<promise_type>::from_promise(*this)};
        }
        void unhandled_exception() { terminate(); }
        void return_void() {}
    };

    struct iterator // : std::iterator<input_iterator_tag, T>
    {
        using iterator_category = input_iterator_tag;
        using value_type        = T;
        using difference_type   = ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        coroutine_handle<promise_type> _coro;
        bool _done;

        iterator(coroutine_handle<promise_type> coro, bool done)
            : _coro(coro) , _done(done) {}

        iterator& operator++()
        {
            _coro.resume();
            _done = _coro.done();
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            return _done == other._done;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        const T& operator*() const
        {
            return _coro.promise()._current_value;
        }

        const T* operator->() const
        {
            return &(operator*());
        }
    };

    iterator begin()
    {
        _handle.resume();
        return iterator{_handle, _handle.done()};
    }

    iterator end()
    {
        return iterator{_handle, true};
    }
};

// Note we can use the same generator<T> for all coroutines because they
// basically behave all alike (yield keeping a value in the promise).

template <typename T>
generator<T> seq() noexcept
{
    for (T i = {};; ++i)
        co_yield i;
}

template <typename T>
generator<T> take_until(generator<T>& g, T limit) noexcept
{
    for (auto&& v: g)
        if (v < limit)
            co_yield v;
        else
            break;
}

template <typename T>
generator<T> multiply(generator<T>& g, T factor) noexcept
{
    for (auto&& v: g)
        co_yield v * factor;
}

template <typename T>
generator<T> add(generator<T>& g, T addend) noexcept
{
    for (auto&& v: g)
        co_yield v + addend;
}

int main()
{
    auto s = seq<int>();
    auto t = take_until(s, 10);
    auto m = multiply(t, 2);
    auto a = add(m, 110);

    cout << accumulate(a.begin(), a.end(), 0) << endl;
    return 0;
}
