# Inline Exceptions

A (semi-serious) solution to the "exception vs. return `std::expected`" war for error handling in C++!

This header-only library turns any exception-based function into an `std::expected`-like based function.

How does it work? Say you have a function `foo()` which uses exceptions. Say you want do do different things for `ExceptionA`, `ExceptionB` and any other exception.

Usually, you'd have to write something like this:

```
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
```
using MyTry = InlineTry<ExceptionA, ExceptionB, void>;
auto res_or_exception = MyTry::call(&foo);
if (res.has_value()) {
  // Do something with res.value()
  // NOTE that if foo() returns a reference, then res.value() is a reference.
} else {
  auto exception_variant = res.exception();
  switch (exception_variant.index()) {
    case 0:
      // Do whatever you wanted for ExceptionA
      break;
    case 1:
      // Do whatever you wanted for ExceptionB
      break;
    case 2:
      // Do whatever you wanted for (...)
      break;
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

```
using MyTry = InlineTry<std::exception, void>;
auto res_or_exception = MyTry::call(&foo);
if (res.has_value()) {
  // Do whatever with res.value()
} else {
  auto exception_variant = res.exception();
  switch (exception_variant.index()) {
    case 0:
      cerr << "Got exception with message" << std::get<0>(e_variant).what() << "\n";
      cerr << "(Base type of exception was " << std::get<0>(e_variant).type().name() << ")\n";
      break;
    case 1:
      cerr << "Got unknown exception\n";
      break;
  }
}
```

## Features

- The assumption is that anyone using this library doesn't want exceptions. So
a "bad" access to the `res_or_exception` just aborts immediately:
```
auto res_or_exception = MyTry::call(&foo);
res_or_exception.value(); // If there was an error, this aborts the program.
```

- If the last exception type given is `void`, it adds a `catch(...)` as the
outermost catch. In that case - `MyTry::call(&foo)` is `noexcept`.

- If we catch a single exception - then `res_or_exception.exception()` will
return the exception instead of a `variant`.

- If a function returns a reference - then `res_or_exception.value()` is a
reference.

- If a function returns `void` - then `.value()` can't be called
(`.has_value()` still works as expected)

- You can wrap any function and call it later:
```
using MyTry = InlineTry<std::exception, void>;
auto wrapped_foo = MyTry::wrap(&foo);
for (...) {
  auto res_or_exception = wrapped_foo(i, j, a, b); // This can't throw! (as long as move constructors are no-throw)
  if (res_or_exception) {
    // do something with res_or_exception.value()
  }
}
```
