# James McNellis CppCon2016 Coroutine examples

## Await examples

The await examples code is explaine by James McNellis in its CppCon 2016 talk [Introduction to C++
Coroutines](https://youtu.be/ZTqHjjm86Bw).
Basically it details how to the return object of a coroutine:
- Can be used to resume the coroutine from the caller function.
- Defines the associated `promise_type` that defines coroutine behaviour.
  In this examples the `promise_type` is defined as a nested class of the coroutine return type
  instead of using a `coroutine_traits` specialization.

1. [await1.cpp](./await/await1.cpp) defines a `counter()` coroutine that can be resumed from the caller.
   Introduces the builtin awaiters like `std::suspend_always` and `std::suspend_never`.
1. [await2.cpp](./await/await2.cpp) extends the previous example by using several coroutines and one caller.
1. [await3.cpp](./await/await3.cpp) shows how to return values from a coroutine:
   - The `promise_type` must keept the return value as a member variable and implement the
     `return_value()` method to set the value (called from `co_return`).
   - The `promise_type::final_suspend()` method must return a `std::suspend_always` awaiter
     to prevent return member variable destruction before the caller retrieves it.
   - The example introduces a `resumable_thing::get()` method to retrieve the return value.
1. [await4.cpp](./await/await4.cpp) extend the previous example by introducing a
   `promise_type::await_transform()` method to transform the await expression provided by
   the coroutine (in this case a `std::suspend_always` awaiter) into an *ad hoc* awaiter.
   The awaiter `await_resume()` method return value is received by the caller on the `co_await` returned.
   But in the example, an operator overloading to cast the awaiter into the expected return
   type (an `int` in this case) is used. That is because `main()` cannot be a coroutine and thus the `co_await` operator
   cannot be there.
   Note that a `co_await` operator overload can provide the same functionality as `promise_type::await_transform()`.

## [Future example](./future/future.cpp)

This example shows how to use any type as a coroutine return type by specializing `coroutine_traits`.
In this case the `std::future` is used as the return type. The corresponding `promise_type` is derived from `std::promise`.
An overload of the `co_await` operator is provided to create a naive awaiter for the `std::future` that spawns a new
thread that waits on it.
Microsoft provided an experimental implementation of [both these features](https://raw.githubusercontent.com/microsoft/STL/5762e6bcaf7f5f8b5dba0a9aabf0acbd0e335e80/stl/inc/future) that is now deprecated.

## Yield examples

Introduces the specifics of the `co_yield` operator and how to use it to create a generator (coroutines return types
associated with yield are conventionally called generators).

1. [yield1.cpp](./yield/yield1.cpp) shows how to use `co_yield` to return values from a coroutine.
   On `co_yield` the `promise_type` method `yield_value()` method is called to set the value to be returned
   and define a suitable awaiter (in this case `std::suspend_always`).
   This value is often retrieved by the caller using *ad hoc* iterators but in this case the generator defines
   custom methods (`move_next()` and `current_value()`) to iterate and retrieve the value.
1. [yield2.cpp](./yield/yield2.cpp) extends the previous example by introducing a `iterator` that will
   hide the generator coroutine complexity from the caller. Again the coroutine is suspend each time a value
   is set and resumed by the caller on demand.
1. [yield3.cpp](./yield/yield3.cpp) extends the previous example by showing how the same generator can be recicled
   to service several coroutines. This way a pipeline behaviour can be achieved without generating a complex state
   machine.
