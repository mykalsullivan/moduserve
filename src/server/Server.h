//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "ConnectionManager.h"
#include "MessageProcessor.h"
#include "BroadcastManager.h"
#include "UserManager.h"
#include "UserAuthenticator.h"
#include <atomic>
#include <condition_variable>

class Server {
    friend class ConnectionManager;
    friend class MessageProcessor;
    friend class BroadcastManager;
    friend class UserManager;
    friend class UserAuthenticator;
public:
    explicit Server(int argc, char *argv[]);
    ~Server() = default;

private:
    std::atomic<bool> m_Running;
    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;

    ConnectionManager m_ConnectionManager;
    MessageProcessor m_MessageHandler;
    BroadcastManager m_BroadcastManager;
    UserManager m_UserManager;
    UserAuthenticator m_UserAuthenticator;

public:
    int run();
    void stop();
    [[nodiscard]] bool isRunning() { return m_Running; }
};