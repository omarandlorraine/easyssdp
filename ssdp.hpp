#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <future>
#include <thread>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace ssdp {

constexpr int ssdp_port = 1900;
constexpr const char* ssdp_mcast_addr = "239.255.255.250";
constexpr int ssdp_mcast_port = 1900;

auto search_target_all() -> std::string
{
    return "ST: ssdp:all";
}

auto search_target_uuid(const char* const uuid) -> std::string
{
    std::string st = "ST: uuid:";
    st += uuid;
    return st;
}

auto get_field(const std::string result, const std::string prefix) -> std::optional<std::string>
{
    std::istringstream iss(result);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.compare(0, prefix.length(), prefix) == 0) {
            std::string rest = line.substr(prefix.length());
            std::size_t start = prefix.length();
            std::size_t end = line.find_last_not_of(" \t\n\r\f\v");

            if (end != std::string::npos) {
                return line.substr(prefix.length(), end - start + 1);
            }
        }
    }
    return {};
}

auto get_usn(const std::string result)
{
    return get_field(result, "USN: ");
}

auto get_search_target(const std::string result)
{
    return get_field(result, "ST: ");
}

auto get_location(const std::string result)
{
    return get_field(result, "LOCATION: ");
}

class Search {
public:
    Search(std::string search_target, int seconds, void (&callback)(std::string))
        : callback(callback)
        , seconds(seconds)
        , search_target(search_target)
    {
        cancelled = false;
    }

    ~Search() {
	    cancel();
    }

    void go()
    {
        task = std::async(std::launch::async,
            &(Search::discover_task),
            this,
            std::ref(cancelled));
    }

    void cancel()
    {
        cancelled = true;
    }

    void (&callback)(std::string);

    int seconds;

    std::string search_target;

private:
    std::atomic_bool cancelled;
    std::future<int> task;

    static auto discover_task(void* search, const std::atomic_bool& cancelled) -> int
    {
        auto search_obj = static_cast<Search*>(search);
        // Create a UDP socket
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return 1;
        }

        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK); // non blocking

        // Set socket options to allow broadcast
        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0) {
            std::cerr << "Error setting socket options" << std::endl;
            close(sockfd);
            return 1;
        }

        // Set up destination address
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ssdp_mcast_port);
        addr.sin_addr.s_addr = inet_addr(ssdp_mcast_addr);

        std::string mx = "MX: " + std::to_string(search_obj->seconds) + "\r\n";
        std::string search_msg = "M-SEARCH * HTTP/1.1\r\n"
                                 "Host: 239.255.255.250:1900\r\n"
                                 "Man: \"ssdp:discover\"\r\n";
        search_msg += mx;
        search_msg += search_obj->search_target;
        search_msg += "\r\n\r\n";

        // Send SSDP search message
        if (sendto(sockfd, search_msg.c_str(), search_msg.length(), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "Error sending SSDP search message" << std::endl;
            close(sockfd);
            return 1;
        }

        // Receive responses
        while (!search_obj->cancelled) {
		std::vector<char> buffer(1024);

            ssize_t bytes_received = recv(sockfd, buffer.data(), buffer.size(), 0);
            if (bytes_received > 0) {
		    buffer.resize(bytes_received);
		    buffer.push_back('\0'); // Add null terminator to treat buffer as C-string
		    std::string response(buffer.data(), bytes_received);
		    search_obj->callback(std::move(response));
	    } else if (bytes_received == 0) {
                std::cout << "No more responses" << std::endl;
                break; 
	    } if (bytes_received < 0) {
		    std::this_thread::sleep_for(std::chrono::milliseconds(10));
	    }
        }

        // Close socket
        close(sockfd);

        return 0;
    }
};

}
