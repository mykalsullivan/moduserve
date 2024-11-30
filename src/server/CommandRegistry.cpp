//
// Created by msullivan on 11/29/24.
//

#include "CommandRegistry.h"
#include "commands/Command.h"
#include "../Logger.h"
#include <dlfcn.h>
#include <stdexcept>

// #include "commands/StopCommand.h"
// #include "commands/HelpCommand.h"

CommandRegistry::CommandRegistry(std::barrier<> &serviceBarrier)
{
    // Wait for all services to be initialized
    serviceBarrier.arrive_and_wait();

    std::string commandLibPath = "/home/msullivan/Development/GitHub/ChatApplication/";

    // Load built-in commands
    //m_Commands.emplace("stop", std::make_shared<StopCommand>());
    //m_Commands.emplace("help", std::make_shared<HelpCommand>());

    // Load custom commands
}

void CommandRegistry::loadCommand(const std::string &libPath)
{
    void *handle = dlopen(libPath.c_str(), RTLD_LAZY);

    if (!handle)
    {
        LOG(LogLevel::ERROR, "Failed to open library: " + std::string(dlerror()))
        return;
    }

    using CommandFactoryFunction = Command *();
    auto factoryFunction = reinterpret_cast<CommandFactoryFunction *>(dlsym(handle, "createCommand"));

    if (!factoryFunction)
    {
        LOG(LogLevel::ERROR, "Failed to load command: " + std::string(dlerror()))
        dlclose(handle);
        return;
    }

    auto command = factoryFunction();
    if (!command)
    {
        LOG(LogLevel::ERROR, "Failed to create command instance")
        dlclose(handle);
        return;
    }

    std::string commandName = command->name();

    registerCommand(commandName, [factoryFunction] { return factoryFunction(); });
    LOG(LogLevel::INFO, "Loaded command: \"" + commandName + '\"')
}

void CommandRegistry::registerCommand(const std::string &name, const CommandFactory &factory)
{
    m_CommandFactories[name] = factory;
}

Command *CommandRegistry::createCommand(const std::string& name)
{
    if (m_CommandFactories.find(name) != m_CommandFactories.end())
        return m_CommandFactories[name]();
    throw std::runtime_error("Command not found: " + name);
}