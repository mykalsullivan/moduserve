//
// Created by msullivan on 11/10/24.
//

#include "MessageSubsystem.h"
#include "common/PCH.h"
#include "common/Connection.h"
#include "server/Server.h"
#include "server/commands/Command.h"
#include "server/Signal.h"

int MessageSubsystem::init()
{
    // Register signals with the SignalManager
    server.signalManager().registerSignal("onReceive", onReceive);

    // Connect the signal to slot
    onReceive.connect(&MessageSubsystem::handleMessage);
    return 0;
}

// This will need to do other stuff in the future
void MessageSubsystem::handleMessage(const Connection &sender, const std::string &message)
{
    // ... do stuff ...

    // Handle message
    parseMessage(sender, message);
}

// This needs to parse messages and commands properly (commands use '/' delimiter)
void MessageSubsystem::parseMessage(const Connection &sender, const std::string &message)
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
        logMessage(LogLevel::DEBUG, "Received keepalive from client @ " + sender.getIP() + ':' + std::to_string(sender.getPort()));
    }
    else
    {
        logMessage(LogLevel::INFO,  + "Client @ " + sender.getIP() + ':' + std::to_string(sender.getPort()) + " sent: \"" + message + '\"');
        auto signal = server.signalManager().getSignal<Signal<const Connection &, const std::string &>>("onBroadcast");
        if (signal) signal->emit(sender, message);
    }
}
