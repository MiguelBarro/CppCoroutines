# WinRT Coroutine examples

- [Simple client & server](#simple-client--simple-server)
- [client & server](#client--server)
- [C++/WinRT projection](#cwinrt-projection)

## [Simple client](./winrt_simple_client.cpp) & [Simple server](./winrt_simple_server.cpp)

This example simplifies the behavior of the ASIO performance tests
[introduced by Gor Nishanov](../Gor/README.md#client--server) at CppCon2017 (an upgrade of the
[classic ASIO machine state approach](../Gor/README.md#classic_client--classic_server)) using
[C++/WinRT coroutine framework](../README.md#references).

It is a simplification because the WinRT C++ projection relies on the
[process thread pool](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pools)
which cannot be coerced to use a specific number of threads (as in the ASIO performance tests).

## [client](./winrt_client.cpp) & [server](./winrt_server.cpp)

This example implements an extension of the WinRT framework ([threadpool_winrt.h](./threadpool_winrt.h))
to support coroutine resumption on a custom thread pool.

This makes it possible to implement a client-server pair matching the ASIO performance tests but sticking
to the C++/WinRT coroutine strategy.

### Threadpool extension explanation

The [threadpool_winrt.h](./threadpool_winrt.h) header relies on the implementation layer of the
C++/WinRT projection (`namespace winrt::impl`). Therefore, it is advisable to keep the
`USE_SDK_HEADERS` CMake option set to `OFF` (see [local projection](#cwinrt-projection)).
This ensures that the projection sources remain compatible with the extension.

To integrate a custom thread pool with the C++/WinRT coroutine framework:

- A [thread pool callback environment](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pools#thread-pool-architecture)
  must be created. This Windows API supersedes the old
  [Vista thread pool API](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pooling).

  Basically, the *callback environment* encapsulates the thread pool, resource cleanup and job dispatching, timer and IO
  integration.
  [Kenny Kerr](https://github.com/kennykerr) has excellent
  [articles](https://learn.microsoft.com/en-us/archive/msdn-magazine/2011/august/windows-with-c-the-windows-thread-pool-and-work)
  on this subject.

  The extension uses a RAII wrapper, `threadpool_winrt::environment`, which manages all the details. The user only
  specifies the fix number of threads, as in the ASIO performance tests.

- To integrate the thread pool with the coroutine framework, a new `threadpool_winrt::pool_promise`
  aware of the new *callback environment*, is associated with those coroutines with the signature:
  ```c++
    winrt::Windows::Foundation::IAsyncXXX coroutine_name(threadpool_winrt::environment&, Args...)
  ```
  via coroutine traits specialization.
  The `threadpool_winrt::pool_promise` derives from `winrt::impl::promise_base`, preserving all WinRT
  coroutine capabilities.

  The promise defines a constructor that matches the coroutine signature arguments so it can capture
  the *callback environment* reference.

- The new `threapool_winrt::pool_promise` base class relies on new `threadpool_winrt::pool_awaiter`
  derived from `winrt::impl::await_adapter`, which overrides `await_suspend` to resume
  coroutines on the thread pool.
  
  As in the standard C++/WinRT coroutine framework, if a completed coroutine is `co_await`ed,
  it is not suspended (this is the purpose of the `promise_base::supending` member whose operation
  is preserved in the extension).

- The C++/WinRT coroutine framework relies on global `operator co_await` definitions to associate
  the `IAsyncXXX` interfaces with the appropriate `await_adapter`.
  [Raymond Chen](https://github.com/oldnewthing) explains this [here](https://devblogs.microsoft.com/oldnewthing/20191218-00/?p=103221).

  Unfortunately, the global `operator co_await` is not suitable in this case, because it ignores
  the coroutine arguments (the *callback environment* reference).

  The `threadpool_winrt::pool_promise` defines `await_transform` methods to associate the new `pool_awaiter`s.
  The C++/WinRT coroutine framework uses `await_transform` to detect user-canceled coroutines during `co_await` and
  return immediately (by throwing). The new `await_transform` methods preserve this behavior.

  [SFINAE](https://en.cppreference.com/w/cpp/language/sfinae.html) must be used to assure only awaitable types
  are processed by the new `await_transform` methods (the other types are delegated to the framework implementations).

## C++/WinRT projection

The examples can use the Windows SDK provided C++/WinRT projection headers; however, to
avoid SDK versioning issues, a locally provided projection is favoured.
This setting can be modify with the [CMakeLists'](./CMakeLists.txt) `USE_SDK_HEADERS` option.

The projection was generated using [cppwinrt](https://github.com/microsoft/cppwinrt) tool.
Ideally, the project should include only the types used in the examples together with their dependencies.
This cannot be accomplished with the tool, despite of the misleading docs:
- The `/include` and `/exclude` arguments work only with components (`/component`).
- For ordinary projections, all types defined in the `/input` metadata files are projected.
- The tool does not track dependencies. Only types explicitly provided via `/input` are
  projected.

Using [mdmerge](https://learn.microsoft.com/en-us/windows/win32/midl/mdmerge-and-metadata-files)
it is possible to split a metadata file into separate files associated with different namespaces.
`cppwinrt` can process one of these files to project a specific namespace, but as noted
above, dependencies will be ignored and the generated sources will fail to compile (dangling
headers inclusion)

As a workaround, instead of restricting the projection, it is possible to trim it.
The [projection.ps1](./projection.ps1) relies on the Visual Studio compiler capability to generate dependency trees
[/sourceDependencies](https://learn.microsoft.com/en-us/cpp/build/reference/sourcedependencies?view=msvc-170).
Only the headers actually required by the examples are preserved.
