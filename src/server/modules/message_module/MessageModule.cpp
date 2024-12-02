//
// Created by msullivan on 11/10/24.
//

#include "MessageModule.h"
#include "common/PCH.h"
#include "common/Connection.h"
#include "server/Server.h"
#include "server/commands/Command.h"

int MessageModule::init()
{
    // Register signals with the SignalManager
    REGISTER_SIGNAL("onReceive", onReceive);

    // Connect the signal to slot
    connectSignal(onReceive, &MessageModule::handleMessage);
    return 0;
}

// This will need to do other stuff in the future
void MessageModule::handleMessage(const OldConnection &sender, const std::string &message)
{
    // ... do stuff ...

    // Handle message
    parseMessage(sender, message);
}

// This needs to parse messages and commands properly (commands use '/' delimiter)
void MessageModule::parseMessage(const OldConnection &sender, const std::string &message)
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
    {
        logMessage(LogLevel::Debug, "Received keepalive from client @ " + sender.ip() + ':' + std::to_string(
                sender.port()));
    }
    else
    {
        logMessage(LogLevel::Info, + "Client @ " + sender.ip() + ':' + std::to_string(sender.port()) + " sent: \"" + message + '\"');
        auto signal = GET_SIGNAL("onBroadcast", const OldConnection &, const std::string &);
        if (signal) signal->emit(sender, message);
    }
}
