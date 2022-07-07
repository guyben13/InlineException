#include "inline_exception.h"

#include <iostream>

struct A {};
struct B {};

struct C {};

int foo() {
  static int i = 0;
  ++i;
  if (i == 1) {
    throw A{};
  }
  if (i == 2) {
    throw B{};
  }
  if (i == 5) {
    throw std::out_of_range("Just some text");
  }
  if (i == 7) {
    throw C{};
  }
  return i;
}

int main() {
  try {
    for (size_t i = 1; i < 10; ++i) {
      auto res = inline_try::InlineTry<A, B, std::exception, void>::wrap(&foo);
      if (res.index() == 0) {
        std::cerr << "Value: " << std::get<0>(res) << '\n';
      } else {
        std::cerr << i << " Error: " << res.index() << '\n';
        if (res.index() == 3) {
          std::cerr << "   type: " << std::get<3>(res).type().name()
                    << " what: " << std::get<3>(res).what() << '\n';
        }
      }
    }
  } catch (C&) {
    std::cerr << "Caught C!\n";
  }
  return 0;
}

