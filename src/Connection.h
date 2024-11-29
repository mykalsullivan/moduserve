//
// Created by msullivan on 11/9/24.
//

#pragma once
#include <string>
#include <netinet/in.h>
#include <chrono>

class Connection {
public:
    Connection();
    virtual ~Connection();

protected:
    int m_SocketFD;
    sockaddr_in m_Address;
    std::chrono::steady_clock::time_point m_LastActivityTime;

public:
    bool createSocket();
    void setSocket(int fd);
    virtual void setAddress(sockaddr_in address);
    [[nodiscard]] int getSocket() const { return m_SocketFD; }
    [[nodiscard]] sockaddr_in &getAddress() { return m_Address; }
    [[nodiscard]] std::string getIP() const;
    [[nodiscard]] int getPort() const;

    void enableKeepalive() const;

    [[nodiscard]] bool sendMessage(const std::string &message) const;
    [[nodiscard]] std::string receiveMessage();

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isInactive(int timeout) const;
    void updateLastActivityTime();
};