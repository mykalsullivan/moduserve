//
// Created by msullivan on 11/29/24.
//

#include "BroadcastSubsystem.h"
#include "server/Server.h"
#include "server/subsystems/connection_subsystem/ConnectionSubsystem.h"
#include "common/Connection.h"
#include "common/Logger.h"
#include <unordered_map>

int BroadcastSubsystem::init()
{
    return 0;
}

void BroadcastSubsystem::broadcastMessage(const Connection &sender, const std::string &message)
{
    logMessage(LogLevel::DEBUG, "Attempting to broadcast message...");

    auto connectionSubsystem = dynamic_cast<ConnectionSubsystem *>(Server::instance().getSubsystem("ConnectionSubsystem"));
    std::unordered_map<int, Connection *> connections;

    for (size_t i = 0; i < connectionSubsystem->size(); i++)
        connections.emplace(i, connectionSubsystem->get(i));

    logMessage(LogLevel::DEBUG, "Retrieved " + std::to_string(connectionSubsystem->size()) + " connections; attempting to broadcast message...");
    for (auto &[fd, connection] : connections)
    {
        // Skip server and sender
        if (fd == connectionSubsystem->serverFD() || fd == sender.getFD() || !connection) continue;

        // Attempt to send message
        if (connection->sendData(message))
            logMessage(LogLevel::DEBUG, "Sent message to client @ " + connection->getIP() + ':' + std::to_string(connection->getPort()));
        else
            logMessage(LogLevel::ERROR, "Failed to send message to client @ " + connection->getIP() + ':' + std::to_string(connection->getPort()));
    }
}