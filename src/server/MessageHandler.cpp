//
// Created by msullivan on 11/10/24.
//

#include "MessageHandler.h"
#include "UserManager.h"
#include "Logger.h"
#include "Server.h"

#define LOG(logLevel, message) Logger::instance().logMessage(logLevel, message);

MessageHandler::MessageHandler(Server &server) : m_Server(server)
{
    Logger::instance().logMessage(LogLevel::DEBUG, "Started message handler");
    m_MessageProcessingThread = std::thread([this]() {
        Logger::instance().logMessage(LogLevel::DEBUG, "(MessageHandler) Started message processing thread");

        while (m_Server.isRunning())
        {
            std::pair<Connection *, std::string> message;
            {
                std::unique_lock lock(m_Server.connectionManager().getMessageQueueMutex());
                m_Server.connectionManager().getCV().wait(lock, [this] {
                    return !m_Server.connectionManager().getMessageQueue().empty();
                }); // This is causing the MessageHandler to never stop since it is blocking

                if (!m_Server.isRunning()) break;

                message = m_Server.connectionManager().getMessageQueue().front();
                m_Server.connectionManager().getMessageQueue().pop();
            }
            handleMessage(message.first, message.second);
        }
        Logger::instance().logMessage(LogLevel::DEBUG, "(MessageHandler) Stopped message processing thread");
    });
}

MessageHandler::~MessageHandler()
{
    Logger::instance().logMessage(LogLevel::DEBUG, "Stopping message handler...");
    if (m_MessageProcessingThread.joinable())
        m_MessageProcessingThread.join();
    Logger::instance().logMessage(LogLevel::DEBUG, "Stopped message handler");
}

// This will need to do other stuff in the future
void MessageHandler::handleMessage(Connection *sender, const std::string &message)
{
    if (sender != nullptr) {
        Logger::instance().logMessage(LogLevel::DEBUG,  + "Client " + std::to_string(sender->getSocket()) + " (" + sender->getIP() + ':' + std::to_string(sender->getPort()) + " ) sent message: \"" + message + '\"');

        // Add message to user's history
        m_Server.userManager()[sender->getSocket()]->addMessageToHistory(message);

        // Handle message
        parseMessage(sender, message);
    }
}

// This will eventually pass messages into a dedicated message processor/parser
void MessageHandler::parseMessage(Connection *sender, const std::string &message)
{
    if (message == "/quit")
    {
        // Handle client quit logic here...
        //Logger::instance().logMessage(LogLevel::INFO, "Client " + std::to_string(sender->getSocket()) + " has quit.");
        m_Server.connectionManager().removeConnection(sender->getSocket());

    }
    else if (message == "/stop")
        m_Server.stop();
    else
    {
        //Logger::instance().logMessage(LogLevel::INFO, "Received message: \"" + message + "\" from client " + std::to_string(sender->getSocket()));
        broadcastMessage(sender, message);
    }
}

void MessageHandler::broadcastMessage(Connection *sender, const std::string &message)
{
    Logger::instance().logMessage(LogLevel::INFO, "Broadcasting message: \"" + message + "\" from " + std::to_string(sender->getSocket()) + " (" + sender->getIP() + ':' + std::to_string(sender->getPort()) + ')');

    std::string senderUsername = "server";//m_UserManager[sender->getSocket()]->getUsername();

    for (auto it = m_Server.connectionManager().begin(); it != m_Server.connectionManager().end(); )
    {
        // Skip the server
        if (it->first == m_Server.connectionManager().getServerSocketID()) {
            ++it;
            continue;
        }

        // Skip the sender
        if (it->first == sender->getSocket()) {
            ++it;
            continue;
        }

        // Check if the connection is still valid
        if (it->second->isValid()) {
            if (!it->second->sendMessage(message)) {
                // Handle failure, e.g. remove the connection
                Logger::instance().logMessage(LogLevel::ERROR, "Failed to send message to client " + std::to_string(it->first) + " (" + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + ')');
                m_Server.connectionManager().removeConnection(it->first); // remove invalid connection
                // Note: Remove will invalidate the iterator, but we will not use 'it' again, so we continue
                it = m_Server.connectionManager().begin(); // Restart the loop after removing the connection
            }
            else {
                Logger::instance().logMessage(LogLevel::DEBUG, "Sent message to client " + std::to_string(it->first) + " (" + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + ')');
                ++it;
            }
        }
        else {
            // If the connection is no longer valid, remove it
            Logger::instance().logMessage(LogLevel::INFO, "Client " + std::to_string(it->first) + " (" + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + ") is no longer valid, removing...");
            m_Server.connectionManager().removeConnection(it->first);
            it = m_Server.connectionManager().begin(); // Restart the loop after removing the connection
        }
    }
}