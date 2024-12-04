//
// Created by msullivan on 11/10/24.
//

#include "MessageProcessor.h"
#include <common/PCH.h>
#include <server/modules/NetworkEngine.h>

// Forward declaration(s)
void parseMessage(Connection sender, const std::string &message);

// Static signal definitions
Signal<Connection, const std::string &> MessageProcessor::beforeMessageParse;;
Signal<Connection, const std::string &> MessageProcessor::processMessage;
Signal<Connection, const std::string &> MessageProcessor::afterMessageParse;

void MessageProcessor::init()
{
    processMessage.connect(&onProcessMessage);
}

// This will need to do other stuff in the future
void MessageProcessor::onProcessMessage(Connection sender, const std::string &message)
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

        // CommandProcessor::receiveCommand(std::move(sender), message);
    }
    else if (message == "KEEPALIVE")
        NetworkEngine::onReceiveKeepalive(std::move(sender));
    NetworkEngine::broadcast(sender, message);
}

// This needs to parse messages and commands properly (commands use '/' delimiter)
void processMessage(Connection sender, const std::string &message)
{

}
