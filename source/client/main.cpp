#include "NettyGritty_C.hpp"
#include <iostream>
#include <string>
#include <algorithm>

#define IP_ADDRESS "127.0.0.1"
#define PORT 8080

int main()
{
    try
    {
        std::string input;
        NetworkInitializer client;

        // Setup client socket
        client.OpenSocket(SocketDomain::IPv4, SocketType::Stream);
        client.SetSocketAddress(IPAddress(IP_ADDRESS), Port(PORT));

        // Register event callbacks BEFORE connecting
        client.OnConnect([](GritSocket& socket) {
            std::cout << "Connected to server!" << std::endl;
        });

        client.OnDataReceived([](GritSocket& socket, const char* data, size_t length) {
            std::string message(data, length);
            std::cout << "Server response: " << message << std::endl;
        });

        client.OnDisconnect([](GritSocket& socket) {
            std::cout << "Disconnected from server" << std::endl;
        });

        client.OnError([](GritSocket& socket, int errorCode, const std::string& errorMsg) {
            std::cerr << "Error " << errorCode << ": " << errorMsg << std::endl;
        });

        // Connect to server
        client.Connect();

        bool shouldClose = false;
        while (!shouldClose)
        {
            std::cout << "Enter message (or 'Exit' to quit): ";
            std::getline(std::cin, input);

            // Clean up input
            input.erase(std::remove_if(input.begin(), input.end(), [](char c) {
                return c == '\n' || c == '\r' || c == '\t';
            }), input.end());

            if (input == "Exit" || input == "exit")
            {
                shouldClose = true;
                break;
            }

            if (!input.empty())
            {
                client.Send(input);
                client.Poll(100); // Poll for response
            }
        }

        client.Close();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Client error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
