//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Subsystem.h"
#include <string>

// Forward declaration(s)
class ConnectionManager;
class BroadcastManager;
class CommandRegistry;
class Connection;
class Message;

class MessageProcessor : public Subsystem {
public:
    MessageProcessor(ConnectionManager &connectionManager,
                    BroadcastManager &broadcastManager,
                    CommandRegistry &commandRegistry);
    ~MessageProcessor() override = default;

private:
    ConnectionManager &m_ConnectionManager;
    CommandRegistry &m_CommandRegistry;
    BroadcastManager &m_BroadcastManager;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "MessageProcessor"; }

    void handleMessage(Connection &sender, const std::string &message);

private:
    void parseMessage(Connection &sender, const std::string &message) const;
};
