//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "ClientConnection.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

class Client {
    Client();
    ~Client();

public:
    // Singleton instance method
    static Client &instance();

    // Delete copy constructor and assignment operators
    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

private:
    std::atomic<bool> m_Running;
    ClientConnection *m_Connection = nullptr;
    std::string m_Username;

public:
    // Runtime
    int init(int argc, char **argv);
    int run(int argc, char **argv);
    void stop();

    // Server connection
    bool connectToServer(const std::string &ip, int port, int timeout);
    [[nodiscard]] bool sendMessage(const std::string &message) const;
    [[nodiscard]] std::string receiveMessage() const;

    // User authentication
    bool authenticate();
};