# C++ Coroutine Examples

This is a collection of C++ coroutine examples mostly based on the following CppCon talks:
1. [James McNellis "Introduction to C++ Coroutines"](https://youtu.be/ZTqHjjm86Bw)
1. [Gor Nishanov "Naked coroutines live (with networking)"](https://youtu.be/UL3TtTgt3oU)
   Gor uploaded some of its talk files to [github](https://github.com/GorNishanov/await.git).
1. [Kenny Kerr & James McNellis "Putting Coroutines to Work with the Windows Runtime"](https://youtu.be/v0SjumbIips)

The examples are available as [github action artifacts](https://github.com/MiguelBarro/CppCoroutines/actions/workflows/ci.yml)
for the windows and ubuntu platforms without external dependencies (besides the OS).

CMake is the build framework of choice. It is fine tuned for GCC, VC++ and Clang. The workflow runs clang on linux and
also the builtin [clang-cl](https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170) on windows.

For the Gor Nishanov conference examples [boost asio](https://www.boost.org/doc/libs/1_62_0/doc/html/boost_asio.html)
library is required. The github action uses Asio 1.10.8 (Boost 1.62) to match the state of the art at the talk time.

Back then, it was expected that [C++ Extensions for Networking](https://cplusplus.github.io/networking-ts/draft.pdf)
would be readily available in the future, though its optional nature thwarted adoption in Windows platform.

If [nuget](https://www.nuget.org/) is available in the system the CMake framework will download Asio's sources
automatically. Nuget used to be available in all Github runners but support for ubuntu seems to be discontinued on **Noble
Numbat**. The main issue is that nuget is a .NET framework/desktop tool.
On linux it can only be run by [mono](https://www.mono-project.com/). Porting nuget to .NET Core does not seem to be
in Microsoft roadmap.

## Index

1. [McNellis examples](./basics/README.md)
1. [Gor Nishanov](./Gor/README.md)

## References

Documents:
1. [C++20 standard with coroutines](https://github.com/GorNishanov/await/blob/master/c%2B%2B20-with-coroutines.pdf)
1. [N4775. Working Draft, C++ Extensions for Coroutines](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2018/n4775.pdf)
1. [p0057r5. Wording for coroutines](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0057r5.pdf)

Blogs:
1. [Lewiss Baker. Coroutine Theory](https://lewissbaker.github.io/2017/09/25/coroutine-theory)
1. [Lewiss Baker. Understanding operator co_await](https://lewissbaker.github.io/2017/11/17/understanding-operator-co-await)
1. [Lewiss Baker. Understanding the promise type](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type)
1. [Raymond Chen. C++ coroutines: Getting started with awaitable objects](https://devblogs.microsoft.com/oldnewthing/20191209-00/?p=103195)
1. [Raymond Chen. C++ coroutines: The mental model for coroutine promises](https://devblogs.microsoft.com/oldnewthing/20210329-00/?p=105015)
1. [Raymond Chen. Debugging coroutine handles: The Microsoft Visual C++ compiler, clang, and gcc](https://devblogs.microsoft.com/oldnewthing/20211007-00/?p=105777)
1. [Raymond Chen. Debugging coroutine handles: Looking for the source of a one-byte memory corruption](https://devblogs.microsoft.com/oldnewthing/20220930-00/?p=107233)
1. [Raymond Chen. Speculation on the design decisions that led to the common ABI for C++ coroutines](https://devblogs.microsoft.com/oldnewthing/20220103-00/?p=106109)

Talks:
1. [James McNellis "Introduction to C++ Coroutines"](https://youtu.be/ZTqHjjm86Bw)
1. [Kenny Kerr & James McNellis "Putting Coroutines to Work with the Windows Runtime"](https://youtu.be/v0SjumbIips)
1. [Gor Nishanov "Naked coroutines live (with networking)"](https://youtu.be/UL3TtTgt3oU)
1. [Gor Nishanov “Nano-coroutines to the Rescue! (Using Coroutines TS, of Course)”](https://youtu.be/j9tlJAqMV7U)
1. [Andreas Weis "Deciphering C++ Coroutines Part 1 - Andreas Weis"](https://youtu.be/J7fYddslH0Q) 
1. [Andreas Weis "Deciphering C++ Coroutines Part 2 - Andreas Weis"](https://youtu.be/qfKFfQSxvA8) 
