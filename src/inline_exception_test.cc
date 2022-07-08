#include "inline_exception.h"

#include <iostream>

struct A {};
struct B {};

struct C {};

int& foo() {
  static int i = 0;
  ++i;
  std::cerr << "Curr i = " << i << "\n";
  if (i == 1) {
    std::cerr << "Throwing A\n";
    throw A{};
  }
  if (i == 2) {
    std::cerr << "Throwing B\n";
    throw B{};
  }
  if (i == 5) {
    std::cerr << "Throwing out_of_range\n";
    throw std::out_of_range("Just some text");
  }
  if (i == 7) {
    std::cerr << "Throwing C\n";
    throw C{};
  }
  std::cerr << "Function succeeded\n";
  return i;
}

int main() {
  using MyTry = inline_try::InlineTry<A, B, std::exception, void>;
  for (size_t i = 0; i < 10; ++i) {
    auto res = MyTry::call(&foo);
    if (res.has_value()) {
      std::cerr << "Value: " << res.value() << '\n';
      res.value() += 1;
    } else {
      const auto& exception = res.exception();
      std::cerr << " Caught: " << exception.index() << '\n';
      if (exception.index() == 2) {
        std::cerr << "   type: " << std::get<2>(exception).type().name()
                  << " what: " << std::get<2>(exception).what() << '\n';
      }
    }
  }
  return 0;
}

