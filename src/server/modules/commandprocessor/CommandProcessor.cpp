//
// Created by msullivan on 12/7/24.
//

#include "CommandProcessor.h"
#include "server/modules/logger/Logger.h"
#include "server/commands/ServerCommand.h"
#include <sstream>
#include <memory>

#ifndef _WIN32
#include <dlfcn.h>
#else
#endif

namespace
{
	std::unordered_map<std::type_index, std::shared_ptr<ServerCommand>> m_Commands;
    std::mutex m_Mutex;
}

void CommandProcessor::onCommandAdded(ServerCommand &command)
{

}

void CommandProcessor::init()
{

}

void CommandProcessor::registerCommand(std::shared_ptr<ServerCommand> command)
{

}

void CommandProcessor::loadCommandFromLib(const std::string &libPath)
{
    void *handle = dlopen(libPath.c_str(), RTLD_LAZY);

    if (!handle)
    {
        Logger::log(LogLevel::Error, "Failed to open library: " + std::string(dlerror()));
        return;
    }

    using CommandFactoryFunction = ServerCommand *();
    auto factoryFunction = reinterpret_cast<CommandFactoryFunction *>(dlsym(handle, "createCommand"));

    if (!factoryFunction)
    {
        Logger::log(LogLevel::Error, "Failed to load command: " + std::string(dlerror()));
        dlclose(handle);
        return;
    }

    auto command = factoryFunction();
    if (!command)
    {
        Logger::log(LogLevel::Error, "Failed to create command instance");
        dlclose(handle);
        return;
    }

    std::string commandName = "";
    CommandManager::instance().registerCommand(std::make_shared<ServerCommand>(factoryFunction()));
    Logger::log(LogLevel::Info, "Loaded command: \"" + commandName + '\"');
}

void CommandProcessor::execute(std::vector<std::string> &input)
{
	auto &commandName = input[0];

//        if (args.size() > 2)
//        {
//            // This should parse the command arguments and pass them into the command
//            if (args[1] == "bf")
//            {
//                if (args.size() > 3)
//                {
//                    if (args[2] == "execute")
//                        BFModule::receivedCode(std::move(connection), args[3]);
//                    else if (args[2] == "retrieve_pointer_address")
//                        BFModule::retrieveCurrentPointerAddress(connection);
//                    else if (args[2] == "retrieve_pointer_value")
//                        BFModule::retrieveCurrentPointerValue(connection);
//                    else if (args[2] == "retrieve_tape_values")
//                        BFModule::retrieveTapeValues(connection, std::stoi(args[3]), std::stoi(args[4]));
//                }
//            }
//            else
//                // Print usage information for the command
//                sendData(std::move(connection), "Missing command arguments");
//        }
//        else
//            sendData(std::move(connection), "Please specify a command");


}