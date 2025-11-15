#pragma once
#ifndef NETTYGRITTY_H
    #include "NettyGritty.h"
#endif


// ============================================================================
// RAII Network Initializer
// ============================================================================

class NetworkInitializer {
public:
    NetworkInitializer() {
#ifdef NETTYGRITTY_WINDOWS
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
        }
#endif
    }
    
    ~NetworkInitializer() {
#ifdef NETTYGRITTY_WINDOWS
        WSACleanup();
#endif
    }
    
    // Prevent copying
    NetworkInitializer(const NetworkInitializer&) = delete;
    NetworkInitializer& operator=(const NetworkInitializer&) = delete;
};