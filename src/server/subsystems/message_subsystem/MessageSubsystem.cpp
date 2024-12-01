//
// Created by msullivan on 11/10/24.
//

#include "MessageSubsystem.h"
#include "common/PCH.h"
#include "common/Connection.h"
#include "server/Server.h"
#include "server/subsystems/connection_subsystem/ConnectionSubsystem.h"
#include "server/subsystems/broadcast_subsystem/BroadcastSubsystem.h"
#include "server/subsystems/command_subsystem/CommandSubsystem.h"
#include "server/commands/Command.h"

int MessageSubsystem::init()
{
    return 0;
}

// This will need to do other stuff in the future
void MessageSubsystem::handleMessage(const Connection &sender, const std::string &message)
{
    LOG(LogLevel::INFO,  + "Client @ " + sender.getIP() + ':' + std::to_string(sender.getPort()) + " sent: \"" + message + '\"');

    // ... do stuff ...

    // Handle message
    parseMessage(sender, message);
}

// This needs to parse messages properly
void MessageSubsystem::parseMessage(const Connection &sender, const std::string &message) const
{
    auto commandSubsystem = dynamic_cast<CommandSubsystem *>(Server::instance().getSubsystem("CommandSubsystem"));
    auto it = commandSubsystem->find(message);
    if (it != commandSubsystem->end())
    {
        auto command = it->second.operator()();
        command->execute(message);
        delete command;
    }
    else
    {
        auto broadcastSubsystem = dynamic_cast<BroadcastSubsystem *>(Server::instance().getSubsystem("BroadcastSubsystem"));
        broadcastSubsystem->broadcastMessage(sender, message);
    }
}
