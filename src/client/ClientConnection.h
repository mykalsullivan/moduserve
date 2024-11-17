//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Connection.h"
#include <thread>
#include <atomic>

class ClientConnection : public Connection {
public:
    ClientConnection();
    ~ClientConnection() override;

private:
    std::atomic<bool> m_KeepaliveRunning;
    std::thread m_KeepaliveThread;
    int m_KeepaliveInterval = 15;

    std::atomic<bool> m_MessagePolling;
    std::thread m_MessagePollingThread;
    int m_MessagePollingInterval = 100;

public:
    bool createAddress(const std::string &ip, int port);
    int connectToServer();
    void closeConnection();

    void startMessagePollingThread();
    void stopMessagePollingThread();

    void startKeepaliveThread();
    void stopKeepaliveThread();

    [[nodiscard]] std::string receiveMessage() const;
};