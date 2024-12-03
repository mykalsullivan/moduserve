//
// Created by msullivan on 11/10/24.
//

#include "MessageProcessor.h"
#include <common/PCH.h>
#include <server/modules/networkengine/NetworkEngine.h>

// Forward declaration(s)
void parseMessage(Connection sender, const std::string &message);

int MessageProcessor::init()
{
    ServerSignal::connect(&NetworkEngine::onDataReceive, &handleMessage);
    return 0;
}

// This will need to do other stuff in the future
void MessageProcessor::handleMessage(Connection sender, const std::string &message)
{
    // ... do stuff ...

    // Handle message
    parseMessage(sender, message);
}

// This needs to parse messages and commands properly (commands use '/' delimiter)
void parseMessage(Connection sender, const std::string &message)
{
    if (message[0] == '/')
    {
//        auto commandSubsystem = dynamic_cast<CommandManager *>(Server::instance().subsystem("CommandManager"));
//        auto it = commandSubsystem->find(message);
//        if (it != commandSubsystem->end())
//        {
//            auto command = it->second.operator()();
//            command->execute(message);
//            delete command;
//        }
    }
    else if (message == "KEEPALIVE")
        NetworkEngine::onReceiveKeepalive(sender);
    NetworkEngine::broadcast(sender, message);
}
