//
// Created by msullivan on 11/9/24.
//

#pragma once
#include <string>
#include <chrono>
#include <entt/entt.hpp>

// For Linux support
#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

class OldConnection {
public:
    OldConnection();
    virtual ~OldConnection();

protected:
    u_long m_FD;
    sockaddr_in m_Address;
    std::chrono::steady_clock::time_point m_LastActivityTime;

public:
    bool createSocket();
    void setSocket(int fd);
    virtual void setAddress(sockaddr_in address);
    [[nodiscard]] u_long fd() const { return m_FD; }
    [[nodiscard]] std::string ip() const;
    [[nodiscard]] int port() const;

    [[nodiscard]] bool sendData(const std::string &data) const;
    [[nodiscard]] std::string receiveData();

    [[nodiscard]] bool hasPendingData() const;
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isInactive(int timeout) const;
    void updateLastActivityTime();
};

using Connection = entt::entity;

struct SocketComponent {
    u_long fd;
    sockaddr_in address;
};

struct SettingsComponent {

};

struct InfoComponent {
    std::chrono::steady_clock::time_point lastActivityTime;
};

struct ServerComponent {};

namespace Blah {
    bool createSocket(int id);
    void setSocket(int id);
    void setAddress(int id, sockaddr_in address);
    [[nodiscard]] u_long fd(int id);
    [[nodiscard]] std::string ip(int id);
    [[nodiscard]] int port(int id);
    bool sendData(int id, const std::string &data);
    [[nodiscard]] std::string receiveData(int id);
    bool hasPendingData(int id);
    [[nodiscard]] bool isValid(int id);
    [[nodiscard]] bool isInactive(int id);
    void updateLastActivityTime(int id);
}