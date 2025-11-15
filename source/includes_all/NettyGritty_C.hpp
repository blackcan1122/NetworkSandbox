#pragma once
#ifndef NETTYGRITTY_H
#include "NettyGritty.h"
#endif
#include <string>
#include <functional> // Fix: was <function>
#include <cstring>
#include <vector>
#include <memory>
#include <iostream>


enum class SocketDomain
{
    IPv4,
    IPv6,
    UnixLocal  // Will fallback on unsupported platforms
};

enum class SocketType
{
    Stream,
    Datagram,
    Raw
};

enum class SocketFlags
{
    None = 0,
    NonBlocking = 1 << 0,
    CloseOnExec = 1 << 1
};

class IPAddress
{
private:
    std::string addr_string;
    NettyGritty::in_addr_t addr_binary;
    bool is_valid;

public:
    IPAddress() : addr_string(""), is_valid(false)
    {
        std::memset(&addr_binary, 0, sizeof(addr_binary));
    }

    IPAddress(const std::string &ip) : is_valid(false)
    {
        std::memset(&addr_binary, 0, sizeof(addr_binary));
        if (inet_pton(AF_INET, ip.c_str(), &addr_binary) == 1)
        {
            addr_string = ip;
            is_valid = true;
        }
    }

    IPAddress &operator=(const std::string &ip)
    {
        is_valid = false;
        std::memset(&addr_binary, 0, sizeof(addr_binary));
        if (inet_pton(AF_INET, ip.c_str(), &addr_binary) == 1)
        {
            addr_string = ip;
            is_valid = true;
        }
        else
        {
            addr_string = "";
        }
        return *this;
    }

    bool IsValid() const { return is_valid; }

    std::string ToString() const { return addr_string; }

    const NettyGritty::in_addr_t &GetBinary() const { return addr_binary; }

    uint32_t ToUint32() const
    {
        return is_valid ? ntohl(addr_binary.s_addr) : 0;
    }

    static IPAddress FromUint32(uint32_t ip)
    {
        IPAddress result;
        result.addr_binary.s_addr = htonl(ip);
        char str[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &result.addr_binary, str, INET_ADDRSTRLEN))
        {
            result.addr_string = str;
            result.is_valid = true;
        }
        return result;
    }
};

class Port
{
private:
    uint16_t port_number;
    bool is_valid;

public:
    Port() : port_number(0), is_valid(false) {}

    Port(uint16_t port) : port_number(port), is_valid(port > 0) {}

    Port(int port) : port_number(0), is_valid(false)
    {
        if (port > 0 && port <= 65535)
        {
            port_number = static_cast<uint16_t>(port);
            is_valid = true;
        }
    }

    Port &operator=(uint16_t port)
    {
        port_number = port;
        is_valid = (port > 0);
        return *this;
    }

    Port &operator=(int port)
    {
        if (port > 0 && port <= 65535)
        {
            port_number = static_cast<uint16_t>(port);
            is_valid = true;
        }
        else
        {
            port_number = 0;
            is_valid = false;
        }
        return *this;
    }

    bool IsValid() const { return is_valid; }

    uint16_t ToUint16() const { return port_number; }

    uint16_t ToNetworkOrder() const { return htons(port_number); }

    static Port FromNetworkOrder(uint16_t network_port)
    {
        return Port(ntohs(network_port));
    }

    std::string ToString() const
    {
        return is_valid ? std::to_string(port_number) : "";
    }
};


// Forward declarations
class GritSocket;

// Event callback types
using OnConnectCallback = std::function<void(GritSocket&)>;
using OnDisconnectCallback = std::function<void(GritSocket&)>;
using OnDataReceivedCallback = std::function<void(GritSocket&, const char* data, size_t length)>;
using OnErrorCallback = std::function<void(GritSocket&, int errorCode, const std::string& errorMsg)>;
using OnNewClientCallback = std::function<void(std::shared_ptr<GritSocket>)>;

class GritSocket
{
private:
    NettyGritty::socket_t __socket = NettyGritty::INVALID_SOCKET_VALUE;
    NettyGritty::sockaddr_in_t __sockaddr = {};
    bool __isInitialized = false;
    bool __isNonBlocking = false;
    
    OnConnectCallback __onConnect;
    OnDisconnectCallback __onDisconnect;
    OnDataReceivedCallback __onDataReceived;
    OnErrorCallback __onError;
    
    char __receiveBuffer[4096];

public:
    GritSocket() = default;

    GritSocket(NettyGritty::socket_t _socket, NettyGritty::sockaddr_in_t _sockaddr, bool nonBlocking = false)
        : __socket(_socket), __sockaddr(_sockaddr), __isNonBlocking(nonBlocking)
    {
        if (NettyGritty::is_valid_socket_t(_socket))
        {
            __isInitialized = true;
            std::memset(__receiveBuffer, 0, sizeof(__receiveBuffer));
        }
    }

    void OnConnect(OnConnectCallback callback) { __onConnect = callback; }
    void OnDisconnect(OnDisconnectCallback callback) { __onDisconnect = callback; }
    void OnDataReceived(OnDataReceivedCallback callback) { __onDataReceived = callback; }
    void OnError(OnErrorCallback callback) { __onError = callback; }

    bool operator==(const NettyGritty::socket_t _socket) const
    {
        return __socket == _socket;
    }

    GritSocket& operator=(NettyGritty::socket_t _bs)
    {
        if (NettyGritty::is_valid_socket_t(_bs))
        {
            __socket = _bs;
            __isInitialized = true;
        }
        return *this;
    }

    void SetSockAddr(NettyGritty::sockaddr_in_t _sockaddr)
    {
        if (NettyGritty::is_valid_socket_t(__socket))
        {
            __sockaddr = _sockaddr;
            __isInitialized = true;
        }
    }

    int Send(const void* data, size_t length, int flags = 0)
    {
        if (!__isInitialized) return -1;
        
        int result = send(__socket, static_cast<const char*>(data), length, flags);
        if (result < 0)
        {
            TriggerError(NettyGritty::get_last_error_t(), "Send failed");
        }
        return result;
    }

    int Send(const std::string& data, int flags = 0)
    {
        return Send(data.c_str(), data.length(), flags);
    }

    // Poll for incoming data and trigger callbacks
    void PollEvents()
    {
        if (!__isInitialized) return;

        std::memset(__receiveBuffer, 0, sizeof(__receiveBuffer));
        int bytesReceived = recv(__socket, __receiveBuffer, sizeof(__receiveBuffer) - 1, 0);

        if (bytesReceived > 0)
        {
            if (__onDataReceived)
            {
                __onDataReceived(*this, __receiveBuffer, bytesReceived);
            }
        }
        else if (bytesReceived == 0)
        {
            TriggerDisconnect();
            Close();
        }
        else
        {
            int error = NettyGritty::get_last_error_t();
            if (error != EWOULDBLOCK && error != EAGAIN)
            {
                TriggerError(error, "Receive failed");
                Close();
            }
        }
    }

    void Close()
    {
        if (__isInitialized && __socket != NettyGritty::INVALID_SOCKET_VALUE)
        {
            NettyGritty::close_socket_t(__socket);
            __socket = NettyGritty::INVALID_SOCKET_VALUE;
            __isInitialized = false;
        }
    }

    void TriggerConnect()
    {
        if (__onConnect) __onConnect(*this);
    }

    void TriggerDisconnect()
    {
        if (__onDisconnect) __onDisconnect(*this);
    }

    void TriggerError(int errorCode, const std::string& msg)
    {
        if (__onError) __onError(*this, errorCode, msg);
    }

    NettyGritty::socket_t GetBinarySocket() const { return __socket; }
    bool IsInitialized() const { return __isInitialized; }
    const NettyGritty::sockaddr_in_t& GetSockAddr() const { return __sockaddr; }
};

class NetworkInitializer
{
private:
    GritSocket __socket;
    NettyGritty::sockaddr_in_t __serverAddr = {};

    SocketDomain __SocketDomain;
    SocketType __SocketType;
    SocketFlags __SocketFlags;

    bool __isValid = false;
    bool __isServer = false;
    bool __isBound = false;
    bool __isListening = false;
    
    // For server mode
    std::vector<std::shared_ptr<GritSocket>> __clients;
    std::vector<NettyGritty::pollfd_t> __pollFds;
    OnNewClientCallback __onNewClient;
    OnErrorCallback __onError;

public:
    NetworkInitializer()
    {
#ifdef NETTYGRITTY_WINDOWS
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
        }
#endif
    }

    ~NetworkInitializer()
    {
        Close();
#ifdef NETTYGRITTY_WINDOWS
        WSACleanup();
#endif
    }

    void OpenSocket(SocketDomain _Domain, SocketType _Type, SocketFlags _Flag = SocketFlags::None)
    {
        __SocketDomain = _Domain;
        __SocketType = _Type;
        __SocketFlags = _Flag;

        int af = GetAddressFamily(_Domain);
        int st = GetSocketType(_Type);
        int fl = static_cast<int>(_Flag);

#ifdef NETTYGRITTY_LINUX
        if (fl & static_cast<int>(SocketFlags::NonBlocking))
        {
            st |= SOCK_NONBLOCK;
        }
        if (fl & static_cast<int>(SocketFlags::CloseOnExec))
        {
            st |= SOCK_CLOEXEC;
        }
#endif

        NettyGritty::socket_t sock = socket(af, st, 0);

        if (sock == NettyGritty::INVALID_SOCKET_VALUE)
        {
            std::cerr << NettyGritty::get_last_error_t() << std::endl;
            throw std::runtime_error("Failed to create socket");
        }

        NettyGritty::set_non_blocking_t(sock, (fl & static_cast<int>(SocketFlags::NonBlocking)));
        
        __socket = GritSocket(sock, __serverAddr, (fl & static_cast<int>(SocketFlags::NonBlocking)));
        __isValid = true;
    }

    void SetSocketAddress(Port __Port)
    {
        if (!__isValid)
        {
            throw std::runtime_error("Setup Socket before setting up Address");
        }

        std::memset(&__serverAddr, 0, sizeof(__serverAddr));
        __serverAddr.sin_family = GetAddressFamily(__SocketDomain);
        __serverAddr.sin_addr.s_addr = INADDR_ANY;
        __serverAddr.sin_port = __Port.ToNetworkOrder();
        
        __socket.SetSockAddr(__serverAddr);
    }

    void SetSocketAddress(const IPAddress& _IpAddress, Port __Port)
    {
        if (!__isValid)
        {
            throw std::runtime_error("Setup Socket before setting up Address");
        }

        std::memset(&__serverAddr, 0, sizeof(__serverAddr));
        __serverAddr.sin_family = GetAddressFamily(__SocketDomain);
        __serverAddr.sin_addr = _IpAddress.GetBinary();
        __serverAddr.sin_port = __Port.ToNetworkOrder();
        
        __socket.SetSockAddr(__serverAddr);
    }

    void SetReuseAddr(bool enable = true)
    {
        if (!__isValid)
        {
            throw std::runtime_error("Socket not initialized");
        }

        int opt = enable ? 1 : 0;
        NettyGritty::setsockopt_t(__socket.GetBinarySocket(), SOL_SOCKET, SO_REUSEADDR,
                                   reinterpret_cast<NettyGritty::message_t>(&opt), sizeof(opt));
    }

    // Server-specific methods
    void Bind()
    {
        if (!__isValid)
        {
            throw std::runtime_error("Socket not initialized");
        }

        if (bind(__socket.GetBinarySocket(), (sockaddr*)&__serverAddr, sizeof(__serverAddr)) < 0)
        {
            throw std::runtime_error("Bind failed: " + std::to_string(NettyGritty::get_last_error_t()));
        }
        
        __isBound = true;
        __isServer = true;
    }

    void Listen(int backlog = SOMAXCONN)
    {
        if (!__isBound)
        {
            throw std::runtime_error("Must bind before listening");
        }

        if (listen(__socket.GetBinarySocket(), backlog) < 0)
        {
            throw std::runtime_error("Listen failed: " + std::to_string(NettyGritty::get_last_error_t()));
        }

        // Setup poll for server socket
        NettyGritty::pollfd_t serverPollFd;
        serverPollFd.fd = __socket.GetBinarySocket();
        serverPollFd.events = POLLIN;
        __pollFds.push_back(serverPollFd);

        __isListening = true;
    }

    // Client-specific method
    void Connect()
    {
        if (!__isValid)
        {
            throw std::runtime_error("Socket not initialized");
        }

        if (connect(__socket.GetBinarySocket(), (sockaddr*)&__serverAddr, sizeof(__serverAddr)) < 0)
        {
            throw std::runtime_error("Connect failed: " + std::to_string(NettyGritty::get_last_error_t()));
        }

        __socket.TriggerConnect();
    }

    // Event registration
    void OnNewClient(OnNewClientCallback callback) { __onNewClient = callback; }
    void OnError(OnErrorCallback callback) { __onError = callback; }
    
    // For client mode - register events on main socket
    void OnConnect(OnConnectCallback callback) { __socket.OnConnect(callback); }
    void OnDisconnect(OnDisconnectCallback callback) { __socket.OnDisconnect(callback); }
    void OnDataReceived(OnDataReceivedCallback callback) { __socket.OnDataReceived(callback); }

    // Poll for events (works for both client and server)
    void Poll(int timeoutMs = 100)
    {
        if (__isServer && __isListening)
        {
            PollServer(timeoutMs);
        }
        else if (__isValid)
        {
            PollClient(timeoutMs);
        }
    }

    // Send data (for client mode)
    int Send(const void* data, size_t length, int flags = 0)
    {
        return __socket.Send(data, length, flags);
    }

    int Send(const std::string& data, int flags = 0)
    {
        return __socket.Send(data, flags);
    }

    void Close()
    {
        __clients.clear();
        __pollFds.clear();
        __socket.Close();
        __isValid = false;
        __isServer = false;
        __isBound = false;
        __isListening = false;
    }

    GritSocket& GetSocket() { return __socket; }
    size_t GetClientCount() const { return __clients.size(); }

private:
    void PollServer(int timeoutMs)
    {
        int pollCount = NettyGritty::poll_t(__pollFds.data(), __pollFds.size(), timeoutMs);

        if (pollCount < 0)
        {
            if (__onError)
            {
                __onError(__socket, NettyGritty::get_last_error_t(), "Poll failed");
            }
            return;
        }

        // Check server socket for new connections
        if (__pollFds[0].revents & POLLIN)
        {
            AcceptNewClient();
        }

        // Check client sockets (iterate backwards to safely remove)
        for (int i = static_cast<int>(__clients.size()) - 1; i >= 0; --i)
        {
            if (__clients[i]->IsInitialized())
            {
                __clients[i]->PollEvents();
                
                if (!__clients[i]->IsInitialized())
                {
                    RemoveClient(i);
                }
            }
        }
    }

    void PollClient(int timeoutMs)
    {
        NettyGritty::pollfd_t pollFd;
        pollFd.fd = __socket.GetBinarySocket();
        pollFd.events = POLLIN;

        int pollCount = NettyGritty::poll_t(&pollFd, 1, timeoutMs);

        if (pollCount > 0 && (pollFd.revents & POLLIN))
        {
            __socket.PollEvents();
        }
    }

    void AcceptNewClient()
    {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        NettyGritty::socket_t clientSocket = accept(__socket.GetBinarySocket(), (sockaddr*)&clientAddr, &clientLen);

        if (NettyGritty::is_valid_socket_t(clientSocket))
        {
            NettyGritty::set_non_blocking_t(clientSocket, true);
            
            auto client = std::make_shared<GritSocket>(clientSocket, clientAddr, true);
            
            NettyGritty::pollfd_t clientPollFd;
            clientPollFd.fd = clientSocket;
            clientPollFd.events = POLLIN;
            __pollFds.push_back(clientPollFd);
            
            __clients.push_back(client);

            if (__onNewClient)
            {
                __onNewClient(client);
            }
        }
    }

    void RemoveClient(size_t index)
    {
        if (index < __clients.size())
        {
            __clients.erase(__clients.begin() + index);
            __pollFds.erase(__pollFds.begin() + index + 1);
        }
    }

    int GetAddressFamily(SocketDomain _Domain)
    {
        switch(_Domain)
        {
            case SocketDomain::IPv4:
                return AF_INET;
            
            case SocketDomain::IPv6:
                return AF_INET6;

            case SocketDomain::UnixLocal:
#ifdef NETTYGRITTY_LINUX
                return AF_UNIX;
#else
                return AF_INET;
#endif

            default:
                return AF_INET;
        } 
    }

    int GetSocketType(SocketType _Type)
    {
        switch (_Type)
        {
        case SocketType::Stream:
            return SOCK_STREAM;
        case SocketType::Datagram:
            return SOCK_DGRAM;
        case SocketType::Raw:
            return SOCK_RAW;
        default:
            return SOCK_STREAM;
        }
    }

    NetworkInitializer(const NetworkInitializer &) = delete;
    NetworkInitializer &operator=(const NetworkInitializer &) = delete;
};