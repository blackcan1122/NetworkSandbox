#include "NettyGritty.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <poll.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    NettyGritty::NetworkInitializer netInit;
    NettyGritty::socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (!NettyGritty::is_valid_socket(sockfd))
    {
        int error = NettyGritty::get_last_error();
        std::cerr << "Socket creation failed with error: " << error << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return EXIT_FAILURE;
    }

    // Set socket to non-blocking
    NettyGritty::set_non_blocking(sockfd, true);

    // Enable SO_REUSEADDR to avoid "Address already in use" errors
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    std::cout << serverAddr.sin_addr.s_addr << std::endl;

    if (bind(sockfd, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed with error: " << NettyGritty::get_last_error()
                  << " (" << strerror(errno) << ")" << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return EXIT_FAILURE;
    }

    std::cout << "Server bound successfully to port " << PORT << std::endl;

    if (listen(sockfd, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed with error: " << NettyGritty::get_last_error() << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return EXIT_FAILURE;
    }

    std::cout << "Server listening... Press Ctrl+C to stop." << std::endl;

    std::vector<pollfd> fds;
    pollfd server_fd;
    server_fd.fd = sockfd;
    server_fd.events = POLLIN;
    fds.push_back(server_fd);

    while (true)
    {
        int poll_count = poll(fds.data(), fds.size(), 100);

        if (poll_count < 0)
        {
            std::cerr << "Poll failed with error: " << NettyGritty::get_last_error() << std::endl;
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == sockfd)
                {
                    // Accept new connection
                    sockaddr_in clientAddr;
                    socklen_t clientLen = sizeof(clientAddr);
                    NettyGritty::socket_t clientSocket = accept(sockfd, (sockaddr *)&clientAddr, &clientLen);

                    if (NettyGritty::is_valid_socket(clientSocket))
                    {
                        NettyGritty::set_non_blocking(clientSocket, true);
                        std::cout << "New client connected! Total clients: " << fds.size() << std::endl;
                        
                        pollfd client_fd;
                        client_fd.fd = clientSocket;
                        client_fd.events = POLLIN;
                        fds.push_back(client_fd);
                    }
                }
                else
                {
                    // Handle client data
                    char buffer[BUFFER_SIZE];
                    std::memset(buffer, 0, BUFFER_SIZE);
                    
                    int bytesReceived = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);
                    
                    if (bytesReceived > 0)
                    {
                        std::cout << "Received from client " << fds[i].fd << ": " << buffer << std::endl;
                    }
                    else if (bytesReceived == 0)
                    {
                        std::cout << "Client " << fds[i].fd << " disconnected. Total clients: " << (fds.size() - 2) << std::endl;
                        NettyGritty::close_socket(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                    else
                    {
                        int error = NettyGritty::get_last_error();
                        if (error != EWOULDBLOCK && error != EAGAIN)
                        {
                            std::cerr << "Receive failed from client " << fds[i].fd << " with error: " << error << std::endl;
                            NettyGritty::close_socket(fds[i].fd);
                            fds.erase(fds.begin() + i);
                            --i;
                        }
                    }
                }
            }
        }
    }

    // Cleanup
    for (auto& fd : fds)
    {
        NettyGritty::close_socket(fd.fd);
    }

    return EXIT_SUCCESS;
}