//
// Created by msullivan on 12/7/24.
//

#include "CommandProcessor.h"
#include "server/modules/Logger.h"

#ifndef _WIN32
#include <dlfcn.h>
#else
#endif

void loadCommand(const std::string &libPath)
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

    //std::string commandName = command->name();

    //registerCommand(commandName, [factoryFunction] { return factoryFunction(); });
    //Logger::log(LogLevel::Info, "Loaded command: \"" + commandName + '\"');
}