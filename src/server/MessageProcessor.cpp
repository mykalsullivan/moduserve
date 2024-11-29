//
// Created by msullivan on 11/10/24.
//

#include "MessageProcessor.h"
#include "ConnectionManager.h"
#include "UserManager.h"
#include "../Logger.h"
#include "Server.h"

MessageProcessor::MessageProcessor(Server &server) : m_Server(server)
{}

// This will need to do other stuff in the future
void MessageProcessor::handleMessage(Connection &sender, const std::string &message)
{
    LOG(LogLevel::INFO,  + "Client @ " + sender.getIP() + ':' + std::to_string(sender.getPort()) + " sent: \"" + message + '\"');

    // Add message to user's history
    m_Server.m_UserManager.getUser(sender.getFD())->addMessageToHistory(message);

    // Handle message
    parseMessage(sender, message);
}

// This will eventually pass messages into a dedicated message processor/parser
void MessageProcessor::parseMessage(Connection &sender, const std::string &message)
{
    if (message == "/quit")
    {
        // Handle client quit logic here...
        //LOG(LogLevel::INFO, "Client " + std::to_string(sender.getSocket()) + " has quit->");
        m_Server.m_ConnectionManager.removeConnection(sender.getFD());
    }
    else if (message == "/stop")
        m_Server.stop();
    else if (message == "/" || message == "/help")
        return; // Don't do anything for now. Will send a command list later
    else
    {
        //Logger::instance().logMessage(LogLevel::INFO, "Received message: \"" + message + "\" from client " + std::to_string(sender.getSocket()));
        m_Server.m_BroadcastManager.broadcastMessage(sender, message);
    }
}