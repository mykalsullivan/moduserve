//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "ConnectionManager.h"
#include "UserAuthenticator.h"
#include "UserManager.h"
#include "MessageHandler.h"
#include <atomic>
#include <condition_variable>

class Server {
    friend class ConnectionManager;
    friend class UserManager;
    friend class UserAuthenticator;
    friend class MessageHandler;
public:
    explicit Server(int argc, char *argv[]);
    ~Server() = default;

private:
    std::atomic<bool> m_Running;
    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;

    ConnectionManager m_ConnectionManager;
    UserAuthenticator m_UserAuthenticator;
    UserManager m_UserManager;
    MessageHandler m_MessageHandler;

public:
    int run();
    void stop();
    [[nodiscard]] bool isRunning() { return m_Running; }
};