# Inline Exceptions

A (semi-serious) solution to the "exception vs. return `std::expected`" war for error handling in C++!

This header-only library turns any exception-based function into an `std::expected`-like based function.

How does it work? Say you have a function `foo()` which uses exceptions. Say you want do do different things for `ExceptionA`, `ExceptionB` and any other exception.

Usually, you'd have to write something like this:

```cpp
try {
  Type res = foo();
  // Do something with res
} catch (ExceptionA& a) {
  // Do something with a
} catch (ExceptionB& b) {
  // Do something with b
} catch (...) {
  // Do something
}
```

Now you can instead do:
```cpp
void handle_error(ExceptionA);
void handle_error(ExceptionB);
void handle_error(std::monostate);


using MyTry = InlineTry<ExceptionA, ExceptionB, void>;
auto res = MyTry::call(&foo);
if (res.has_value()) {
  // Do something with res.value()
} else {
  std::visit([](const auto& e) {handle_error(e);}, res.exception());
  // NOTE: res.exception() returns a variant of the exception (almost)
}
```

## Features

- The assumption is that anyone using this library doesn't want exceptions. So
a "bad" access to the `res` just aborts immediately:
```cpp
auto res = MyTry::call(&foo);
res.value(); // If there was an error, this aborts the program.
```

- If the last exception type given is `void`, it adds a `catch(...)` as the
outermost catch. In that case - `MyTry::call(&foo)` is `noexcept`.

  - The `res.error()` variant will have `std::monostate` as the exception value
  for the outermost `catch`.

- If we catch a single exception - then `res.exception()` will return the
exception instead of a `variant`.

- If a function returns a reference - then `res.value()` is a reference.

- If a function returns `void` - then `.value()` can't be called
(`.has_value()` still works as expected)

- You can wrap any function and call it later:
```cpp
using MyTry = InlineTry<Exception, void>;
auto wrapped_foo = MyTry::wrap(&foo);
for (...) {
  auto res = wrapped_foo(i, j, a, b);
  // NOTE that we `catch(...)` (we have `void` as the last exception)
  // Hence, the call to `wrapped_foo` is no-except (the move constructor might
  // throw though)
  if (res) {
    // do something with res.value()
  } else {
    // Handle the error in res.exception()
  }
}
```

### Caveats

Since we need to copy the exception, the exception declared can't be pure-virtual.

And generally, if the actual exception thrown inherits from the exception we
declared, things might not do what we expect (copy constructor of the parent
from the child?)

**But the most common exception we want to catch is `std::exception`, which is pure virtual!**

Hence we have a special case for `std::exception` - if you declare you want to
catch `std::exception`, the actual type in the exception `variant` will be a
wrapper with `.what()` that returns the original exception's `.what()` and a
`.type()` that returns the `typeid` of the original exception.

Similar wrappers can be created and added by the user for custom exception types.

```cpp
using MyTry = InlineTry<std::exception>;
// NOTE: we're catchins a single exception, so `res.exception` is just that exception
auto res = MyTry::call(&foo);
if (res) {
  // Do whatever with res.value()
} else {
  cerr << "Got exception with message" << res.exception().what() << "\n";
  cerr << "(Base type of exception was " << res.exception().type().name() << ")\n";
}
```

