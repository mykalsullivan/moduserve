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
public:
    Client(int argc, char *argv[]);
    ~Client();

private:
    std::atomic<bool> m_Running;
    ClientConnection *m_Connection = nullptr;

    std::string m_Username;
    std::mutex m_Mutex;

public:
    // Server connection
    bool connectToServer(const std::string &ip, int port, int timeout);
    void closeConnection();
    [[nodiscard]] bool sendMessage(const std::string &message) const;
    [[nodiscard]] std::string receiveMessage() const;

    // User authentication
    bool authenticate();

    // User input
    void handleUserInput();
};