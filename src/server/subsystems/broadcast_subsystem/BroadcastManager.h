//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "../Subsystem.h"
#include <string>

// Forward declaration(s)
class ConnectionManager;
class MessageProcessor;
class Connection;

class BroadcastManager : public Subsystem {
public:
    BroadcastManager(ConnectionManager &connectionManager,
                    MessageProcessor &messageProcessor);
    ~BroadcastManager() override = default;

private:
    ConnectionManager &m_ConnectionManager;
    MessageProcessor &m_MessageProcessor;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "BroadcastManager"; }

    void broadcastMessage(Connection &sender, const std::string &message);
};
