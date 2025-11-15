#include "NettyGritty_C.hpp"
#include <iostream>

#define PORT 8080

int main()
{
    try
    {
        NetworkInitializer server;
        
        // Setup server socket
        server.OpenSocket(SocketDomain::IPv4, SocketType::Stream, SocketFlags::NonBlocking);
        server.SetReuseAddr(true);
        server.SetSocketAddress(Port(PORT));
        server.Bind();
        server.Listen();

        std::cout << "Server bound successfully to port " << PORT << std::endl;
        std::cout << "Server listening... Press Ctrl+C to stop." << std::endl;

        // Register event callbacks
        server.OnNewClient([](std::shared_ptr<GritSocket> client) {
            std::cout << "New client connected!" << std::endl;

            // Register callbacks for this client
            client->OnDataReceived([](GritSocket& socket, const char* data, size_t length) {
                std::string message(data, length);
                std::cout << "Received from client: " << message << std::endl;
            });

            client->OnDisconnect([](GritSocket& socket) {
                std::cout << "Client disconnected" << std::endl;
            });

            client->OnError([](GritSocket& socket, int errorCode, const std::string& errorMsg) {
                std::cerr << "Client error " << errorCode << ": " << errorMsg << std::endl;
            });
        });

        server.OnError([](GritSocket& socket, int errorCode, const std::string& errorMsg) {
            std::cerr << "Server error " << errorCode << ": " << errorMsg << std::endl;
        });

        // Main event loop
        while (true)
        {
            server.Poll(100);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}