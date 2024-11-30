//
// Created by msullivan on 11/10/24.
//

#include "MessageProcessor.h"
#include "ConnectionManager.h"
#include "BroadcastManager.h"
#include "CommandRegistry.h"
#include "../../Connection.h"
#include "../../Logger.h"
#include "../commands/Command.h"

MessageProcessor::MessageProcessor(ConnectionManager &connectionManager,
                                   BroadcastManager &broadcastManager,
                                   CommandRegistry &commandRegistry) :
                                    m_ConnectionManager(connectionManager),
                                    m_BroadcastManager(broadcastManager),
                                    m_CommandRegistry(commandRegistry)
{}

// This will need to do other stuff in the future
void MessageProcessor::handleMessage(Connection &sender, const std::string &message)
{
    LOG(LogLevel::INFO,  + "Client @ " + sender.getIP() + ':' + std::to_string(sender.getPort()) + " sent: \"" + message + '\"');

    // ... do stuff ...

    // Handle message
    parseMessage(sender, message);
}

// This needs to parse messages properly
void MessageProcessor::parseMessage(Connection &sender, const std::string &message) const
{
    auto it = m_CommandRegistry.find(message);
    if (it != m_CommandRegistry.end())
    {
        auto command = it->second.operator()();
        command->execute(message);
        delete command;
    }
    else
        m_BroadcastManager.broadcastMessage(sender, message);
}
