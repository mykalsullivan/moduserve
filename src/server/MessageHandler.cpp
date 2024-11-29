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
    LOG(LogLevel::DEBUG,  + "Client " + std::to_string(sender.getFD()) + " (" + sender.getIP() + ':' + std::to_string(sender.getPort()) + " ) sent message: \"" + message + '\"');

    // Add message to user's history
    m_Server.m_UserManager.getUser(sender.getFD())->addMessageToHistory(message);

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
        m_Server.m_ConnectionManager.removeConnection(sender.getFD());
    }
    else if (message == "/stop")
        m_Server.stop();
    else
    {
        //Logger::instance().logMessage(LogLevel::INFO, "Received message: \"" + message + "\" from client " + std::to_string(sender.getSocket()));
        m_Server.m_BroadcastManager.broadcastMessage(sender, message);
    }
}