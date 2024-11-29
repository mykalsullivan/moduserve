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
    int m_FD;
    sockaddr_in m_Address;
    std::chrono::steady_clock::time_point m_LastActivityTime;

public:
    bool createSocket();
    void setSocket(int fd);
    virtual void setAddress(sockaddr_in address);
    [[nodiscard]] int getFD() const { return m_FD; }
    [[nodiscard]] sockaddr_in &getAddress() { return m_Address; }
    [[nodiscard]] std::string getIP() const;
    [[nodiscard]] int getPort() const;

    void enableKeepalive() const;

    [[nodiscard]] bool sendData(const std::string &data) const;
    [[nodiscard]] std::string receiveData();

    [[nodiscard]] bool hasPendingData() const;
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isInactive(int timeout) const;
    void updateLastActivityTime();
};