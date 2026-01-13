# WinRT Coroutine examples

- [Simple client & server](#simple-client--simple-server)

## [Simple client](./winrt_simple_client.cpp) & [Simple server](./winrt_simple_server.cpp)

This example simplifies the behavior of the asio performance tests
[introduced by Gor Nishanov](../Gor/README.md#client--server) in the CppCon2017 (an upgrade of the
[classic asio machine state approach](../Gor/README.md#classic_client--classic_server)) using
[C++/WinRT coroutine framework](../README.md#references).

It is a simplification because the WinRT C++ projection relies on the
[process thread pool](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pools)
which cannot be coerced to use a specific number of threads (as in the asio performance tests).
