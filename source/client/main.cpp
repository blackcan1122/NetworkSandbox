#include "NettyGritty_C.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#define IP_ADDRESS "127.0.0.1"
#define PORT 8080

int main()
{

    std::string Input;

    NetworkInitializer netInit;
    NettyGritty::socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (!NettyGritty::is_valid_socket_t(sockfd))
    {
        int error = NettyGritty::get_last_error_t();
        std::cerr << "Socket creation failed with error: " << error << std::endl;
        return EXIT_FAILURE;
    }

    NettyGritty::sockaddr_in_t sockaddr_other;
    sockaddr_other.sin_family = AF_INET;
    sockaddr_other.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP_ADDRESS, &sockaddr_other.sin_addr) <= 0)
    {
        std::cerr << "Invalid address: " << IP_ADDRESS << std::endl;
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&sockaddr_other, sizeof(sockaddr_other)) < 0)
    {
        int error = NettyGritty::get_last_error_t();
        std::cerr << "Connection failed with error: " << error << std::endl;
        NettyGritty::close_socket_t(sockfd);
        return EXIT_FAILURE;
    }

    std::cout << "Connected to " << IP_ADDRESS << std::endl;
    bool bShouldClose = false;

    while (bShouldClose != true)
    {
        std::cout << "Enter 'Exit' to close the connection..." << std::endl;

        std::cin >> Input;
        Input.erase(std::remove_if(Input.begin(), Input.end(), [](char c)
                                   { return c == '\\' || c == '\n' || c == '\r' || c == '\t'; }),
                    Input.end());

        if (Input == "Exit")
        {
            NettyGritty::close_socket_t(sockfd);
            bShouldClose = true;
        }

        send(sockfd, Input.c_str(), Input.length(), 0);
    }

    return EXIT_SUCCESS;
}
