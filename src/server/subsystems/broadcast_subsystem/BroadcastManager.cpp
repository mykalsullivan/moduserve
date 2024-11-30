//
// Created by msullivan on 11/29/24.
//

#include "BroadcastManager.h"
#include "server/subsystems/connection_manager_subsystem/ConnectionManager.h"
#include "common/Connection.h"
#include "common/Logger.h"

BroadcastManager::BroadcastManager(ConnectionManager &connectionManager,
                                    MessageProcessor &messageProcessor) :
                                    m_ConnectionManager(connectionManager),
                                    m_MessageProcessor(messageProcessor)
{}

int BroadcastManager::init()
{
    return 0;
}

void BroadcastManager::broadcastMessage(Connection &sender, const std::string &message)
{
    for (auto &[fd, connection] : m_ConnectionManager)
    {
        // Skip server and sender
        if (fd == m_ConnectionManager.serverFD() || fd == sender.getFD() || !connection) continue;

        // Attempt to send message
        if (connection->sendData(message))
            LOG(LogLevel::DEBUG, "Sent message to client @ " + connection->getIP() + ':' + std::to_string(connection->getPort()))
        else
            LOG(LogLevel::ERROR, "Failed to send message to client @ " + connection->getIP() + ':' + std::to_string(connection->getPort()))
    }
}