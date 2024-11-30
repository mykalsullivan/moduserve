//
// Created by msullivan on 11/10/24.
//

#include "MessageSubsystem.h"
#include "common/PCH.h"
#include "common/Connection.h"
#include "server/subsystems/connection_subsystem/ConnectionSubsystem.h"
#include "server/subsystems/broadcast_subsystem/BroadcastSubsystem.h"
#include "server/subsystems/command_subsystem/CommandSubsystem.h"

int MessageSubsystem::init()
{
    return 0;
}

// This will need to do other stuff in the future
void MessageSubsystem::handleMessage(Connection &sender, const std::string &message)
{
    LOG(LogLevel::INFO,  + "Client @ " + sender.getIP() + ':' + std::to_string(sender.getPort()) + " sent: \"" + message + '\"');

    // ... do stuff ...

    // Handle message
    parseMessage(sender, message);
}

// This needs to parse messages properly
void MessageSubsystem::parseMessage(Connection &sender, const std::string &message) const
{
//    auto it = m_CommandRegistry.find(message);
//    if (it != m_CommandRegistry.end())
//    {
//        auto command = it->second.operator()();
//        command->execute(message);
//        delete command;
//    }
//    else
//        m_BroadcastManager.broadcastMessage(sender, message);
}
