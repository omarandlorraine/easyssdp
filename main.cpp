#include "ssdp.hpp"

void print_them_out(std::string result)
{
    std::cout << ssdp::get_location(result).value_or("nowhere!") << std::endl;
}

auto main(int argc, char* argv[]) -> int
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--uuid")) {
            ssdp::discover(ssdp::search_target_uuid(argv[++i]), print_them_out);
        } else if (!strcmp(argv[i], "--all")) {
            ssdp::discover(ssdp::search_target_all(), print_them_out);
        } else {
            std::cerr << "unknown command-line argument '" << argv[i] << "'" << std::endl;
        }
        i++;
    }

    return 0;
}
