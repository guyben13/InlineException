run_tests: inline_exception_test.out
	./inline_exception_test.out

inline_exception_test.out: inline_exception_test.cc inline_exception.h
	g++-10 -O3 -Wall -Wextra -Wpedantic -Werror inline_exception_test.cc -o inline_exception_test.out
