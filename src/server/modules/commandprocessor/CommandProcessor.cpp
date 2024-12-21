//
// Created by msullivan on 12/7/24.
//

#include "CommandProcessor.h"
#include "server/modules/logger/Logger.h"
#include "server/modules/networkengine/NetworkEngine.h"
#include "server/commands/ServerCommand.h"
#include <memory>

#ifndef _WIN32
#include <dlfcn.h>
#else
#endif

// Define signals
Signal<const std::string &> CommandProcessor::commandAdded;
Signal<Connection, const std::string &> CommandProcessor::receivedInvalidCommand;

// Define slots
void CommandProcessor::onCommandAdded(const std::string &commandName)
{
    Logger::log(LogLevel::Info, "Registered command: '" + commandName + '\'');
}

void CommandProcessor::onReceivedInvalidCommand(Connection connection, const std::string &command)
{
    std::string ip = NetworkEngine::getIP(connection);
    std::string port = std::to_string(NetworkEngine::getPort(connection));
    Logger::log(LogLevel::Debug, ip + ':' + port + " attempted to execute an invalid or unregistered command: '" + command + '\'');
    NetworkEngine::sendData(connection, "Attempted to execute an invalid or unregistered command");
}

void CommandProcessor::init()
{
    commandAdded.connect(&onCommandAdded);
    receivedInvalidCommand.connect(&onReceivedInvalidCommand);
}

void CommandProcessor::registerCommand(std::shared_ptr<ServerCommand> command)
{
    CommandManager::instance().registerCommand(command);
    commandAdded(command->name());
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

    std::shared_ptr<ServerCommand> command(factoryFunction());
    if (!command)
    {
        Logger::log(LogLevel::Error, "Failed to create command instance");
        dlclose(handle);
        return;
    }

    std::string commandName;
    CommandManager::instance().registerCommand(command);
    Logger::log(LogLevel::Info, "Loaded command: \"" + commandName + '\"');
}

std::vector<std::string> CommandProcessor::stringToVec(const std::string &string)
{
    std::istringstream ss(string);
    std::vector<std::string> vec;
    std::string substr;
    while (ss >> substr) vec.emplace_back(substr);
    return vec;
}

std::string CommandProcessor::vecToString(const std::vector<std::string> &vec)
{
    std::string str;
    for (int i = 0; i < vec.size(); i++)
    {
        str += vec[i];
        if (i != (vec.size() - 1))
            str += ' ';
    }
    return str;
}

void CommandProcessor::execute(Connection connection, const std::string &args)
{
    // 1. Check if a command has been specified; return if not
    auto parsedCommand = stringToVec(args);
    auto &commandName = parsedCommand[0];
    if (parsedCommand.empty()) [[unlikely]] return;

	// 2. Determine and retrieve the command to execute
    auto command = CommandManager::instance().getCommand(commandName);

   	// 3. Execute the command by passing the remaining arguments into it
    if (command != nullptr)
        command->execute(args);
    else
        onReceivedInvalidCommand(connection, args);
}