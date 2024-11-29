//
// Created by msullivan on 11/10/24.
//

#include "MessageHandler.h"
#include "ConnectionManager.h"
#include "UserManager.h"
#include "../Logger.h"
#include "Server.h"

MessageHandler::MessageHandler(Server &server) : m_Server(server)
{}

// This will need to do other stuff in the future
void MessageHandler::handleMessage(Connection &sender, const std::string &message)
{
    LOG(LogLevel::DEBUG,  + "Client " + std::to_string(sender.getSocket()) + " (" + sender.getIP() + ':' + std::to_string(sender.getPort()) + " ) sent message: \"" + message + '\"');

    // Add message to user's history
    m_Server.m_UserManager.getUser(sender.getSocket())->addMessageToHistory(message);

    // Handle message
    parseMessage(sender, message);
}

// This will eventually pass messages into a dedicated message processor/parser
void MessageHandler::parseMessage(Connection &sender, const std::string &message)
{
    if (message == "/quit")
    {
        // Handle client quit logic here...
        //LOG(LogLevel::INFO, "Client " + std::to_string(sender.getSocket()) + " has quit->");
        m_Server.m_ConnectionManager.removeConnection(sender.getSocket());

    }
    else if (message == "/stop")
        m_Server.stop();
    else
    {
        //Logger::instance().logMessage(LogLevel::INFO, "Received message: \"" + message + "\" from client " + std::to_string(sender.getSocket()));
        broadcastMessage(sender, message);
    }
}

void MessageHandler::broadcastMessage(Connection &sender, const std::string &message)
{
    LOG(LogLevel::INFO, "Broadcasting message: \"" + message + "\" from " + std::to_string(sender.getSocket()) + " (" + sender.getIP() + ':' + std::to_string(sender.getPort()) + ')');

    std::string senderUsername = "Server";//m_UserManager[sender.getSocket()].getUsername();

    for (auto it = m_Server.m_ConnectionManager.begin(); it != m_Server.m_ConnectionManager.end();)
    {
        // Skip the server and the sender
        if (it->first == m_Server.m_ConnectionManager.getServerSocketID() || it->first == sender.getSocket())
        {
            ++it;
            continue;
        }

        // Check if the connection is still valid
        if (it->second->isValid())
        {
            if (it->second->sendMessage(message))
            {
                LOG(LogLevel::DEBUG, "Sent message to client " + std::to_string(it->first) + " (" + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + ')');
                ++it;
            }
            else
            {
                // Handle failure, e.g. remove the connection
                LOG(LogLevel::ERROR, "Failed to send message to client " + std::to_string(it->first) + " (" + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + ')');
                m_Server.m_ConnectionManager.removeConnection(it->first); // remove invalid connection
                // Note: Remove will invalidate the iterator, but we will not use 'it' again, so we continue
                it = m_Server.m_ConnectionManager.begin(); // Restart the loop after removing the connection
            }
        }
        else
        {
            // If the connection is no longer valid, remove it
            LOG(LogLevel::INFO, "Client " + std::to_string(it->first) + " (" + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + ") is no longer valid, removing...");
            m_Server.m_ConnectionManager.removeConnection(it->first);
            it = m_Server.m_ConnectionManager.begin(); // Restart the loop after removing the connection
        }
    }
}