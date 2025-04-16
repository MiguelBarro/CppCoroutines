// cl /Zi /EHsc /nologo /std:c++20 /D _WIN32_WINNT=_WIN32_WINNT_WIN10 /D ASIO_STANDALONE /I $Env:TMP/build/asio.1.10.8/build/native/include/ hard2.cpp
// set makeprg=cl\ /Zi\ /EHsc\ /nologo\ /std:c++20\ /D\ _WIN32_WINNT=_WIN32_WINNT_WIN10\ /D\ ASIO_STANDALONE\ /I\ $TMP/build/asio.1.10.8/build/native/include/\ (gi\ %)

#include <coroutine>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <vector>

#include <asio.hpp>
#include <asio/system_timer.hpp>

#include <await_adapters.h>
#include <future_adapter.h>

using namespace std;
using namespace asio;

// The coroutine, we pass the socket by value to keep it alive, using a rvalue reference
// as parameter will fail after suspension (the reference socket will no longer be in the
// stack. Passing it by value the argument will be allocated in heap (bundle with the other
// members of the coroutine state).
future<void> session(ip::tcp::socket s, size_t block_size)
{
    vector<char> buf_(block_size);
    ip::tcp::no_delay no_delay(true);
    s.set_option(no_delay);

    for (;;)
    {
        auto n = co_await async_read_some(s, buffer(buf_.data(), block_size));
        n = co_await async_write(s, buffer(buf_.data(), n));
    }
}

future<void> server(io_service& io, const ip::tcp::endpoint& endpoint, size_t block_size)
{
    ip::tcp::acceptor acceptor(io, endpoint);
    acceptor.listen();

    for (;;)
    {
        ip::tcp::socket socket(io);
        co_await async_accept(acceptor, socket);
        session(std::move(socket), block_size);
    }
}

int main(int argc, const char* argv[])
{
    std::array<std::thread, 0> threads;

    // default args
    static const char* defargs[] = {"hard1", "0.0.0.0", "8888", "128"};
    const char** args = argv;
    if (argc != 4)
        args = defargs;

    cout << args[0] << ' ' << args[1] << ' ' << args[2] << ' ' << args[3] << endl;

    try
    {
        ip::address address(ip::address::from_string(args[1]));
        short port = static_cast<short>(atoi(args[2]));
        size_t block_size = atoi(args[3]);

        io_service io;

        cout << "port: " << port << " block size: " << block_size << " address: " << address << endl;

        server(io, ip::tcp::endpoint(address, port), block_size);

        for(auto& thread : threads)
            thread = std::thread([&io] { io.run(); });

        io.run();

        for(auto& thread : threads)
            thread.join();
    }
    catch (exception &e)
    {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
};
