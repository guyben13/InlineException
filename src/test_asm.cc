#include <chrono>
#include <iostream>

#include "inline_exception.h"

using namespace inline_try;

struct A {};
struct B {};
struct C {};

int foo() {
  static int i = 9;
  i = (i + 1) % 10;
  if (i == 1) {
    throw A{};
  }
  if (i == 2) {
    throw B{};
  }
  if (i == 3) {
    throw C{};
  }
  return i;
}

using MyTry = InlineTry<A, B, void>;

int bar() {
  auto res = MyTry::call(&foo);
  return res.has_value();
}

int dead() {
  try {
    foo();
    return 0;
  } catch (A&) {
    return 1;
  } catch (B&) {
    return 2;
  } catch (...) {
    return 3;
  }
}

int face() {
  try {
    try {
      try {
        foo();
        return 0;
      } catch (A&) {
        return 1;
      }
    } catch (B&) {
      return 2;
    }
  } catch (...) {
    return 3;
  }
}

using TimePoint = std::chrono::high_resolution_clock::time_point;
TimePoint now() { return std::chrono::high_resolution_clock::now(); }
double duration(TimePoint a, TimePoint b) {
  return std::chrono::duration<double>(b - a).count();
}

int main() {
  constexpr size_t N = 1'000'000;
  {
    TimePoint start = now();
    int count = 0;
    for (size_t i = 0; i < N; ++i) {
      count += bar();
    }
    double time = duration(start, now());
    std::cerr << "count= " << count << " time= " << time << '\n';
  }
  {
    TimePoint start = now();
    int count = 0;
    for (size_t i = 0; i < N; ++i) {
      count += dead();
    }
    double time = duration(start, now());
    std::cerr << "count= " << count << " time= " << time << '\n';
  }
  {
    TimePoint start = now();
    int count = 0;
    for (size_t i = 0; i < N; ++i) {
      count += face();
    }
    double time = duration(start, now());
    std::cerr << "count= " << count << " time= " << time << '\n';
  }
  return 0;
}
