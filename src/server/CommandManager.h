//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "commands/CommandRegistry.h"
#include "modules/Logger.h"

#ifndef _WIN32
#include <dlfcn.h>
#else
#endif

class CommandManager {
public:
    CommandManager()
    {
        std::string commandLibPath = "/home/msullivan/Development/GitHub/ChatApplication/";

        // Load built-in commands
        //commandRegistry.add("stop", std::make_shared<StopCommand>());
        //commandRegistry.emplace("help", std::make_shared<HelpCommand>());

        // Load custom commands
    }
    ~CommandManager() = default;

    void loadCommand(const std::string &libPath)
    {
        void *handle = dlopen(libPath.c_str(), RTLD_LAZY);

        if (!handle)
        {
            Logger::log(LogLevel::Error, "Failed to open library: " + std::string(dlerror()));
            return;
        }

        using CommandFactoryFunction = Command *();
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
};