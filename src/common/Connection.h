//
// Created by msullivan on 11/9/24.
//

#pragma once
#include <string>
#include <chrono>

// For Linux support
#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

class Connection {
public:
    Connection();
    virtual ~Connection();

protected:
#ifndef _WIN32
    int m_FD;
#else
    SOCKET m_FD;
#endif
    sockaddr_in m_Address;
    std::chrono::steady_clock::time_point m_LastActivityTime;

public:
    bool createSocket();
    void setSocket(int fd);
    virtual void setAddress(sockaddr_in address);
    [[nodiscard]] int fd() const { return m_FD; }
    [[nodiscard]] std::string ip() const;
    [[nodiscard]] int port() const;

    [[nodiscard]] bool sendData(const std::string &data) const;
    [[nodiscard]] std::string receiveData();

    [[nodiscard]] bool hasPendingData() const;
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isInactive(int timeout) const;
    void updateLastActivityTime();
};
