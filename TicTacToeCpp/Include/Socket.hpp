#pragma once

#include "PacketHeader.hpp"


class Socket
{
public:
    ~Socket();
    Socket();
protected:
    void InitWSA();

    void Cleanup() const;
public:
    virtual void Run() = 0;

protected:
    SOCKET socket_;
    WSADATA wsaData; // contains infos about WIndows sockets impl
    sockaddr_in serverAddr = sockaddr_in(); // for specifying IPv4 socket addresses && protocols

    PacketHeader header;
    char headerBuffer[sizeof(PacketHeader)];
};