SRCS := main.cpp
HDRS := ssdp.hpp
example: $(SRCS) $(HDRS)
	clang++ -o example $(SRCS)

compile_commands.json:
	bear -- make -B -j5

clang-tidy: compile_commands.json
	clang-tidy --checks=modernize-\*,-modernize-avoid-c-arrays,bugprone-\*  $(SRCS) $(HDRS) --p=c++14

clang-tidy-fix: compile_commands.json
	clang-tidy --checks=-\*,modernize-*,readability-*,misc-const-correctness $(SRCS) $(HDRS) --p=c++14 -fix

format:
	clang-format -style=WebKit -i $(SRCS)

