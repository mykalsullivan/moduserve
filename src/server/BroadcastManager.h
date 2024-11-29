//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <string>

// Forward declaration(s)
class Server;
class Connection;

class BroadcastManager {
public:
    explicit BroadcastManager(Server &server);
    ~BroadcastManager() = default;

private:
    Server &m_Server;

public:
    void broadcastMessage(Connection &sender, const std::string &message);
};
