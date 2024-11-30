//
// Created by msullivan on 11/29/24.
//

#include "BroadcastManager.h"
#include "ConnectionManager.h"
#include "../Connection.h"
#include "../Logger.h"

BroadcastManager::BroadcastManager(ConnectionManager &connectionManager,
                                    MessageProcessor &messageProcessor,
                                    std::barrier<> &serviceBarrier) :
                                    m_ConnectionManager(connectionManager),
                                    m_MessageProcessor(messageProcessor)
{
    // Wait for all services to be initialized
    serviceBarrier.arrive_and_wait();
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