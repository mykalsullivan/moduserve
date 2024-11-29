//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Connection.h"
#include <string>

// Forward declaration(s)
class Server;
class Message;

class MessageHandler {
public:
    explicit MessageHandler(Server &server);
    ~MessageHandler() = default;

private:
    Server &m_Server;

public:
    void handleMessage(Connection &sender, const std::string &message);
    void broadcastMessage(Connection &sender, const std::string &message);

private:
    void parseMessage(Connection &sender, const std::string &message);
};
