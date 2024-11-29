//
// Created by msullivan on 11/29/24.
//

#include "BroadcastManager.h"
#include "Server.h"
#include "ConnectionManager.h"
#include "../Connection.h"
#include "../Logger.h"

BroadcastManager::BroadcastManager(Server &server) : m_Server(server)
{}

void BroadcastManager::broadcastMessage(Connection &sender, const std::string &message)
{
    LOG(LogLevel::INFO, "Broadcasting message: \"" + message + "\"");

    for (auto &[fd, connection] : m_Server.m_ConnectionManager)
    {
        // Skip server and sender
        if (fd == m_Server.m_ConnectionManager.m_ServerFD || fd == sender.getFD() || !connection) continue;

        // Attempt to send message
        if (connection->sendData(message))
            LOG(LogLevel::DEBUG, "Sent message to client " + std::to_string(fd) + " (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ')')
        else
            LOG(LogLevel::ERROR, "Failed to send message to client " + std::to_string(fd) + " (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ')')
    }
}