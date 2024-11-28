//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Connection.h"
#include "ConnectionManager.h"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

// Forward declaration(s)
class Server;

class MessageHandler {
public:
    explicit MessageHandler(Server &server);
    ~MessageHandler();

private:
    Server &m_Server;

    std::thread m_MessageProcessingThread;
    std::mutex m_MessageProcessingMutex;

public:
    void handleMessage(Connection *sender, const std::string &message);
    void broadcastMessage(Connection *sender, const std::string &message);

private:
    void parseMessage(Connection *sender, const std::string &message);
};
