SRCS := main.cpp
HDRS := ssdp.hpp
CFLAGS := -std=c++17

example: $(SRCS) $(HDRS)
	clang++ $(CFLAGS) -o example $(SRCS)

compile_commands.json:
	bear -- make -B -j5

clang-tidy: compile_commands.json
	clang-tidy --checks=modernize-\*,-modernize-avoid-c-arrays,bugprone-\*  $(SRCS) $(HDRS) --p=c++17

clang-tidy-fix: compile_commands.json
	clang-tidy --checks=-\*,modernize-*,readability-*,misc-const-correctness $(SRCS) $(HDRS) --p=c++17 --fix

format:
	clang-format -style=WebKit -i $(SRCS)

