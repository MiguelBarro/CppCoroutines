# WinRT Coroutine examples

- [Simple client & server](#simple-client--simple-server)
- [C++/WinRT projection](#cwinrt-projection)

## [Simple client](./winrt_simple_client.cpp) & [Simple server](./winrt_simple_server.cpp)

This example simplifies the behavior of the ASIO performance tests
[introduced by Gor Nishanov](../Gor/README.md#client--server) at CppCon2017 (an upgrade of the
[classic ASIO machine state approach](../Gor/README.md#classic_client--classic_server)) using
[C++/WinRT coroutine framework](../README.md#references).

It is a simplification because the WinRT C++ projection relies on the
[process thread pool](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-pools)
which cannot be coerced to use a specific number of threads (as in the ASIO performance tests).

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
