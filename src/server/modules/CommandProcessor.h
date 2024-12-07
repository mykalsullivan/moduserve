//
// Created by msullivan on 12/7/24.
//

#pragma once
#include "ServerModule.h"
#include "server/commands/ServerCommand.h"
#include <memory>
#include <unordered_map>

class CommandProcessor : public ServerModule {
public signals:
    static Signal<ServerCommand> commandAdded;

public slots:
    static void onCommandAdded(ServerCommand &command);

private:
    static std::unordered_map<std::pair<std::type_index, std::type_index>, std::shared_ptr<ServerCommand>> m_Commands;
    static std::mutex m_Mutex;

public:
    ~CommandProcessor() override;
    void init() override;
    [[nodiscard]] virtual std::vector<std::type_index> requiredDependencies() const;
    [[nodiscard]] virtual std::vector<std::type_index> optionalDependencies() const;

    template<typename Module, typename Command, typename... Args>
    static void addCommand(Args &&... args)
    {
        std::lock_guard lock(m_Mutex);
        const auto type = std::pair(std::type_index(typeid(Module)), std::type_index(typeid(Command)));

        if (m_Commands.contains(type))
            throw std::runtime_error("Command is already registered");
        m_Commands[std::pair<Module, Command>()] = std::make_shared<Module, Command>(std::forward<Args>(args)...);
    }

    template<typename Module, typename Command>
    static std::shared_ptr<Command> getCommand()
    {
        std::lock_guard lock(m_Mutex);
        const auto command = std::pair(std::type_index(typeid(Module), std::type_index(typeid(Command))));
        return m_Commands.contains(command) ? std::dynamic_pointer_cast<Command>(m_Commands[command]) : nullptr;
    }

    template<typename Module, typename Command>
    static bool hasCommand()
    {
        std::lock_guard lock(m_Mutex);
        return getCommand<Module, Command>() ? true : false;
    }

    static std::vector<std::string> parseArgs(const std::string &);
};