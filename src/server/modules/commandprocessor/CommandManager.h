//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <server/commands/ServerCommand.h>
#include <unordered_map>
#include <memory>
#include <mutex>

class CommandManager {
    std::unordered_map<std::string, std::shared_ptr<ServerCommand>> m_Commands;
    std::mutex m_Mutex;

public:
    static CommandManager &instance()
    {
        static CommandManager instance;
        return instance;
    }

    void registerCommand(std::shared_ptr<ServerCommand> command)
    {
        std::lock_guard lock(m_Mutex);

        if (m_Commands.contains(command->name()))
            throw std::runtime_error("This command has already been registered");
        m_Commands[command->name()] = std::move(command);
    }

    // Retrieve a command by type
    std::shared_ptr<ServerCommand> getCommand(const std::string &commandName)
    {
        std::lock_guard lock(m_Mutex);
        return hasCommand(commandName) ? m_Commands[commandName] : nullptr;
    }

    // Returns true if the command is loaded
    bool hasCommand(const std::string &commandName)
    {
        return m_Commands.contains(commandName);
    }
};
