#include <iostream>
#include <cstring>
#include <memory>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace ssdp {

constexpr int ssdp_port = 1900;
constexpr const char * ssdp_mcast_addr = "239.255.255.250";
constexpr int ssdp_mcast_port = 1900;

auto search_target_all() -> std::string {
	return "ssdp:all";
}

auto search_target_uuid(const char * const uuid) -> std::string {
	std::string st = "uuid:";
	st += uuid;
	return st;
}

auto discover(std::string search_target, void (&callback)(std::string)) -> int {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

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

    std::string search_msg =
	"M-SEARCH * HTTP/1.1\r\n"
	"Host: 239.255.255.250:1900\r\n"
	"Man: \"ssdp:discover\"\r\n"
	"MX: 2\r\n"
	"ST: ";
    search_msg += search_target;
    search_msg += "\r\n\r\n";

    // Send SSDP search message
    if (sendto(sockfd, search_msg.c_str(), search_msg.length(), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Error sending SSDP search message" << std::endl;
        close(sockfd);
        return 1;
    }

    // Receive responses
    std::vector<char> buffer(1024);
    while (true) {
        ssize_t bytes_received = recv(sockfd, buffer.data(), buffer.size(), 0);
        if (bytes_received < 0) {
            std::cerr << "Error receiving response" << std::endl;
            break;
        }
        if (bytes_received == 0) {
            std::cout << "No more responses" << std::endl;
            break;
        }
        buffer.resize(bytes_received);
        buffer.push_back('\0'); // Add null terminator to treat buffer as C-string
	std::string response(buffer.data(), bytes_received);
	callback(std::move(response));
        std::cout << "Received response: " << buffer.data() << std::endl;
    }

    // Close socket
    close(sockfd);

    return 0;
}
}
