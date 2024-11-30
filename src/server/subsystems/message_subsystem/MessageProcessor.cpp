//
// Created by msullivan on 11/10/24.
//

#include "MessageProcessor.h"
#include "server/subsystems/connection_manager_subsystem/ConnectionManager.h"
#include "server/subsystems/broadcast_subsystem/BroadcastManager.h"
#include "server/subsystems/command_registry_subsystem/CommandRegistry.h"
#include "common/Connection.h"
#include "common/Logger.h"
#include "server/commands/Command.h"

MessageProcessor::MessageProcessor(ConnectionManager &connectionManager,
                                   BroadcastManager &broadcastManager,
                                   CommandRegistry &commandRegistry) :
                                    m_ConnectionManager(connectionManager),
                                    m_BroadcastManager(broadcastManager),
                                    m_CommandRegistry(commandRegistry)
{}

int MessageProcessor::init()
{
    return 0;
}

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
