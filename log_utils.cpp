// log_utils_cpp
#include "log_utils.hpp"
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

void log_to_file(const std::string& log_line) {
    std::ofstream log_file("server.log", std::ios::app);
    if (log_file.is_open()) {
        log_file << log_line;
        log_file.close();
        std::cout << "Log written to server.log\n";  // optional debug
    } else {
        std::cerr << "Failed to open server.log\n";
    }
}

std::string current_timestamp()
{
    time_t now = time(nullptr);
    char buf[100];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string get_client_ip(int client_fd)
{
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    getpeername(client_fd, (sockaddr*)&client_addr, &len);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    return std::string(ip_str);
}