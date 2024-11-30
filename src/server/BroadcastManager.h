//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <string>
#include <barrier>

// Forward declaration(s)
class ConnectionManager;
class MessageProcessor;
class Connection;

class BroadcastManager {
public:
    BroadcastManager(ConnectionManager &connectionManager,
                    MessageProcessor &messageProcessor,
                    std::barrier<> &serviceBarrier);
    ~BroadcastManager() = default;

private:
    ConnectionManager &m_ConnectionManager;
    MessageProcessor &m_MessageProcessor;

public:
    void broadcastMessage(Connection &sender, const std::string &message);
};
