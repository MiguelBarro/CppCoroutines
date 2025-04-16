// cl /Zi /EHsc /nologo /std:c++20 /D _WIN32_WINNT=_WIN32_WINNT_WIN10 /D ASIO_STANDALONE /I $Env:TMP/build/asio.1.10.8/build/native/include/ over1.cpp
// set makeprg=cl\ /Zi\ /EHsc\ /nologo\ /std:c++20\ /D\ _WIN32_WINNT=_WIN32_WINNT_WIN10\ /D\ ASIO_STANDALONE\ /I\ $TMP/build/asio.1.10.8/build/native/include/\ (gi\ %)

#include <iostream>

#include <asio.hpp>
#include <asio/system_timer.hpp>

#include <asio_future_await.h>
#include <future_adapter.h>

std::future<void> repost(asio::io_service &io, int n)
{
    while (n-- > 0)
    {
        co_await post(io, my_use_future);
        std::cout << "repost " << n << std::endl;
    }
}

int main()
{
    asio::io_service io;
    repost(io, 1'000);
    io.run();
};
