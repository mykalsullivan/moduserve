//
// Created by msullivan on 11/29/24.
//

#include "CommandManager.h"
#include "server/commands/CommandRegistry.h"
#include <stdexcept>

#ifndef _WIN32
#include <dlfcn.h>
#else

#endif

static CommandRegistry commandRegistry;

CommandManager::CommandManager()
{
    std::string commandLibPath = "/home/msullivan/Development/GitHub/ChatApplication/";

    // Load built-in commands
    //commandRegistry.add("stop", std::make_shared<StopCommand>());
    //commandRegistry.emplace("help", std::make_shared<HelpCommand>());

    // Load custom commands
}


//void CommandManager::loadCommand(const std::string &libPath)
//{
//    void *handle = dlopen(libPath.c_str(), RTLD_LAZY);
//
//    if (!handle)
//    {
//        logMessage(LogLevel::ERROR, "Failed to open library: " + std::string(dlerror()));
//        return;
//    }
//
//    using CommandFactoryFunction = Command *();
//    auto factoryFunction = reinterpret_cast<CommandFactoryFunction *>(dlsym(handle, "createCommand"));
//
//    if (!factoryFunction)
//    {
//        logMessage(LogLevel::ERROR, "Failed to load command: " + std::string(dlerror()));
//        dlclose(handle);
//        return;
//    }
//
//    auto command = factoryFunction();
//    if (!command)
//    {
//        logMessage(LogLevel::ERROR, "Failed to create command instance");
//        dlclose(handle);
//        return;
//    }
//
//    std::string commandName = command->name();
//
//    registerCommand(commandName, [factoryFunction] { return factoryFunction(); });
//    logMessage(LogLevel::INFO, "Loaded command: \"" + commandName + '\"');
//}
