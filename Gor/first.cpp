#ifdef USE_CXX_MODULE
    import std_proxy.coroutine;
    import std_proxy.future;
    import std_proxy.iostream;
#else
#   include <coroutine>
#   include <future>
#   include <iostream>
#endif

using namespace std;

template<typename R, typename... Args>
struct coroutine_traits<future<R>, Args...>
{
    struct promise_type
    {
        promise<R> p_;

        suspend_never initial_suspend() { return {}; }
        suspend_never final_suspend() noexcept { return {}; }

        void return_value(R v)
        {
            p_.set_value(v);
        }

        auto get_return_object()
        {
            return p_.get_future();
        }

        void unhandled_exception()
        {
            p_.set_exception(current_exception());
        }
    };
};

future<int> f()
{
    cout << "Hello" << endl;
    co_return 42;
}

int main()
{
    cout << f().get() << endl;
    return 0;
}
