#pragma once
#ifndef NETTYGRITTY_H
#define NETTYGRITTY_H
#define NETTYGRITTY_VERSION "1.0.0"

#include <stdexcept>
#include <string>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    #define NETTYGRITTY_WINDOWS
    
#elif defined(__linux__)
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/select.h>
    #include <netinet/tcp.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <poll.h>
    
    
    #define NETTYGRITTY_LINUX
    
#else
    #error "Unsupported platform - only Windows and Linux are supported"
#endif

namespace NettyGritty {

// ============================================================================
// Platform-agnostic type aliases
// ============================================================================

#ifdef NETTYGRITTY_WINDOWS
    // Socket types
    typedef SOCKET socket_t;
    typedef int socklen_t;
    
    // Address types
    typedef struct sockaddr sockaddr_t;
    typedef struct sockaddr_in sockaddr_in_t;
    typedef struct sockaddr_in6 sockaddr_in6_t;
    typedef struct sockaddr_storage sockaddr_storage_t;
    typedef struct in_addr in_addr_t;
    typedef struct in6_addr in6_addr_t;
    
    // Host/service resolution
    typedef struct addrinfo addrinfo_t;
    
    // Select/polling
    typedef fd_set fd_set_t;
    typedef struct timeval timeval_t;

    // Poll types
    typedef WSAPOLLFD pollfd_t;
    typedef ULONG nfds_t;

    // MessageTypes
    typedef const char* message_t;
    
    // I/O types
    typedef int ssize_t;
    
#elif defined(NETTYGRITTY_LINUX)
    // Socket types
    typedef int socket_t;
    
    // Address types
    typedef struct sockaddr sockaddr_t;
    typedef struct sockaddr_in sockaddr_in_t;
    typedef struct sockaddr_in6 sockaddr_in6_t;
    typedef struct sockaddr_storage sockaddr_storage_t;
    typedef struct in_addr in_addr_t;
    typedef struct in6_addr in6_addr_t;
    
    // Host/service resolution
    typedef struct addrinfo addrinfo_t;
    
    // Select/polling
    typedef fd_set fd_set_t;
    typedef struct timeval timeval_t;

    // Poll types
    typedef struct pollfd pollfd_t;
    typedef nfds_t nfds_t;

    // MessageTypes
    typedef void* message_t;

    // I/O types (already defined on Linux)
    // typedef ssize_t ssize_t;
#endif

// ============================================================================
// Platform-agnostic constants
// ============================================================================

#ifdef NETTYGRITTY_WINDOWS
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
    constexpr int SOCKET_ERROR_VALUE = SOCKET_ERROR;
#elif defined(NETTYGRITTY_LINUX)
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
    constexpr int SOCKET_ERROR_VALUE = -1;
#endif

// ============================================================================
// Platform-agnostic function wrappers
// ============================================================================

inline int close_socket_t(socket_t sock) {
#ifdef NETTYGRITTY_WINDOWS
    return closesocket(sock);
#else
    return close(sock);
#endif
}

inline int get_last_error_t() {
#ifdef NETTYGRITTY_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline int set_non_blocking_t(socket_t sock, bool non_blocking) {
#ifdef NETTYGRITTY_WINDOWS
    u_long mode = non_blocking ? 1 : 0;
    return ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return -1;
    flags = non_blocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(sock, F_SETFL, flags);
#endif
}

inline bool is_valid_socket_t(socket_t sock) {
    return sock != INVALID_SOCKET_VALUE;
}

inline bool socket_error_occurred_t(int result) {
    return result == SOCKET_ERROR_VALUE;
}

inline int poll_t(pollfd_t* fds, nfds_t nfds, int timeout) {
#ifdef NETTYGRITTY_WINDOWS
    return WSAPoll(fds, nfds, timeout);
#else
    return poll(fds, nfds, timeout);
#endif
}

inline bool would_block_error_t() {
#ifdef NETTYGRITTY_WINDOWS
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
}



} // namespace NettyGritty

#endif // NETTYGRITTY_H
