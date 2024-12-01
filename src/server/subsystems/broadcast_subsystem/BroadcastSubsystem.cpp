//
// Created by msullivan on 11/29/24.
//

#include "BroadcastSubsystem.h"
#include "common/Connection.h"
#include "common/Logger.h"
#include "server/Server.h"
#include "server/subsystems/connection_subsystem/ConnectionSubsystem.h"

int BroadcastSubsystem::init()
{
    return 0;
}

void BroadcastSubsystem::broadcastMessage(const Connection &sender, const std::string &message)
{
    //logMessage(LogLevel::DEBUG, "Attempting to broadcast message...");
    auto connectionSubsystem = dynamic_cast<ConnectionSubsystem *>(Server::instance().subsystem("ConnectionSubsystem"));
    for (auto &[fd, connection] : *connectionSubsystem)
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