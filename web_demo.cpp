#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include "log_utils.hpp"
#include "db_utils.hpp"

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"

// function for cpp older than cpp20
bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


bool has_known_extension(const std::string& path) {
    return ends_with(path, ".html") ||
           ends_with(path, ".jpg") || ends_with(path, ".jpeg") ||
           ends_with(path, ".png") || ends_with(path, ".gif") ||
           ends_with(path, ".css") || ends_with(path, ".js") ||
           ends_with(path, ".pdf") || ends_with(path, ".mp4");
}

// Judging types
std::string get_content_type(const std::string &filename)
{
    if(ends_with(filename, ".html")) return "text/html";
    else if (ends_with(filename, ".jpg") || ends_with(filename, ".jpeg")) return"image/jpeg";
    else if (ends_with(filename, ".png")) return"image/png";
    else if (ends_with(filename, ".gif")) return"image/gif";
    else if (ends_with(filename, ".css")) return"text/css";
    else if (ends_with(filename, ".js")) return"application/javascript";
    else if (ends_with(filename, ".pdf")) return"application/pdf";
    else if (ends_with(filename, ".mp4")) return "video/mp4";
    else return "application/octet-stream";
}

/*
// Log
std::string current_timestamp()
{
    time_t now = time(nullptr);
    char buf[100];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M%S", localtime(&now));
    return std::string(buf);
}
*/

// Identify AgentUser
std::string extract_header_value(const std::string& headers, const std::string& key) {
    std::istringstream stream(headers);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find(key) != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos)
                return line.substr(pos + 2);
        }
    }
    return "Unknown";
}

int main(int argc, char*argv[])
{
    // Test db
    if (!init_database("message.db")) std::cerr << "Database initialization failed\n";
    if (!insert_message("message.db", "TestUser", "Hello from SQLite")) std::cerr << "Insert failed in main()\n";

    std::string base_path = "../static/";

    int port=8080;
    if(argc>=2)
    {
        try
        {
            port=std::stoi(std::string(argv[1]));
        }
        catch(const std::invalid_argument& e)
        {
            std::cerr<<"Invalid port: Not A Number! Using default 8080.\n";
            port=8080;
        }
        catch(const std::out_of_range& e)
        {
            std::cerr<<"Invalid port: Number Too Big! Using default 8080.\n";
            port=8080;
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd==-1)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; //Monitor all addresses
    //addr.sin_port = htons(8080);
    addr.sin_port=htons(port);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr))==-1)
    {
        perror("bind");
        return 1;
    }

    if(listen(server_fd, 10)==-1)
    {
        perror("listen");
        return 1;
    }

    std::cout << "HTTP server running on http://localhost:"<<port<<"\n";
    while(true)
    {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd == -1)
        {
            perror("accept");
            continue;
        }

        char buffer[1024] = {0};
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
        if(bytes_read <= 0)
        {
            std::cerr << "Read failed or connection closed\n";
            close(client_fd);
            continue;
        }
        std::cout << "Request:\n" << buffer << "\n";

        std::string request_line(buffer);
        std::istringstream req_stream(request_line);
        std::string method, path, version;
        req_stream >> method >> path >> version;

        std::string raw_request(buffer);
        std::string user_agent = extract_header_value(raw_request, "User-Agent");

        std::string file_path;
        if(path == "/") file_path = base_path+ "Home.html";
        else
        {
            std::string stripped = path.substr(1);
            std::string full_path = base_path + stripped;
            std::ifstream test_file(full_path);
            
            if (test_file.good()) file_path = full_path;
            else 
            {
                std::string try_html = full_path + ".html";
                std::ifstream test_html(try_html);
                if(test_html.good()) file_path = try_html; 
                else file_path = full_path;
            }
        }

        // const char* html_body = "<html><body><h1>Hello, C++ HTTP!</h1></body></html>";

        std::ifstream file(file_path, std::ios::binary);
        std::string body;
        std::string status_line;
        std::string content_type = get_content_type(file_path);

        if(!file)
        {
            status_line = "HTTP/1.1 404 Not Found\r\n";
            std::ifstream not_found_file(base_path + "404.html");
            if(not_found_file)
            {
                std::stringstream nf_stream;
                nf_stream << not_found_file.rdbuf();
                body = nf_stream.str();
                content_type = "text/html";
            }
            else 
            {
                body = "<html><body><h1>404 Not Found</h1></body></html>";
                content_type = "text/html";
            }
        }
        else
        {
            status_line = "HTTP/1.1 200 OK\r\n";
            std::ostringstream content_stream;
            content_stream << file.rdbuf();
            body = content_stream.str();
        }
        std::string status = status_line.find("200") != std::string::npos ? "200 OK" : "404 Not Found";
        std::string client_ip = get_client_ip(client_fd);

        std::string color;
        if (status == "200 OK") color = COLOR_GREEN;
        else if (status == "404 Not Found") color = COLOR_RED;
        else color = COLOR_YELLOW;

        std::string log_line = "["+current_timestamp()+ "] " 
                             + client_ip + " " + method + " "
                             + path + " → " + status + "\n"
                             + " UA:" + user_agent +"\n\n";
        std::cout << color << log_line << COLOR_RESET << "\n";
        // std::cout<<log_line;
        std::ofstream log_file("server.log", std::ios::app);
        log_to_file(log_line);

        // std::stringstream html_stream;
        // html_stream << file.rdbuf();
        // std::string html_body = html_stream.str();

        
        int content_length = body.size();
        
        std::string response = 
        status_line + 
        "Content-Type: " + content_type+ "\r\n"
        "Content-Length: " + std::to_string(content_length) + "\r\n"
        "\r\n";

        ssize_t header_written = write(client_fd, response.c_str(), response.size());
        if(header_written ==-1) std::cerr << "Header write failed\n";

        ssize_t body_written = write(client_fd, body.c_str(), body.size());
        if(body_written ==-1) std::cerr << "Body write failed\n";
        close(client_fd);
    }

    close(server_fd);
    return 0;
}