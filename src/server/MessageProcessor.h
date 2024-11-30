//
// Created by msullivan on 11/10/24.
//

#pragma once
#include <string>
#include <barrier>

// Forward declaration(s)
class ConnectionManager;
class BroadcastManager;
class CommandRegistry;
class Connection;
class Message;

class MessageProcessor {
public:
    explicit MessageProcessor(ConnectionManager &connectionManager,
                                BroadcastManager &broadcastManager,
                                CommandRegistry &commandRegistry,
                                std::barrier<> &serviceBarrier);
    ~MessageProcessor() = default;

private:
    ConnectionManager &m_ConnectionManager;
    CommandRegistry &m_CommandRegistry;
    BroadcastManager &m_BroadcastManager;

public:
    void handleMessage(Connection &sender, const std::string &message);

private:
    void parseMessage(Connection &sender, const std::string &message) const;
};
