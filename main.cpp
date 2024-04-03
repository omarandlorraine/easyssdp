#include "ssdp.hpp"
#include <cstdlib>

void print_them_out(std::string result)
{
    std::cout << ssdp::get_location(result).value_or("nowhere!") << std::endl;
}

auto main(int argc, char* argv[]) -> int
{
    int seconds = 2;
    auto target = ssdp::search_target_all();

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--mx")) {
            seconds = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--uuid")) {
            target = ssdp::search_target_uuid(argv[++i]);
        } else if (!strcmp(argv[i], "--all")) {
            target = ssdp::search_target_all();
        } else {
            std::cerr << "unknown command-line argument '" << argv[i] << "'" << std::endl;
        }
        i++;
    }

    auto search = ssdp::Search(target, seconds, print_them_out);
    search.go();
    std::cin.get();
    search.cancel();

    return 0;
}
