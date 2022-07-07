run_tests: inline_exception_test.out test_asm.out
	./inline_exception_test.out
	./test_asm.out

inline_exception_test.out: inline_exception_test.cc inline_exception.h
	g++-10 -O3 -Wall -Wextra -Wpedantic -Werror -std=c++2a inline_exception_test.cc -o inline_exception_test.out

test_asm.out: test_asm.cc inline_exception.h
	g++-10 -O3 -Wall -Wextra -Wpedantic -Werror -std=c++2a test_asm.cc -o test_asm.out
