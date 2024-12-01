//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <string>
#include <unordered_map>
#include <functional>

// Forward declaration(s)
class Command;

using CommandFactory = std::function<Command *()>;

class CommandRegistry {
    std::unordered_map<std::string, CommandFactory> m_CommandFactories;

public:
    auto begin() const { return m_CommandFactories.begin(); }
    auto end() const { return m_CommandFactories.end(); }
    auto find(const std::string &commandName) const { return m_CommandFactories.find(commandName); }
    [[nodiscard]] bool contains(const std::string &commandName) const { return m_CommandFactories.contains(commandName); }
    [[nodiscard]] bool empty() const { return m_CommandFactories.empty(); }
    [[nodiscard]] size_t size() const { return m_CommandFactories.size(); }

    void registerCommand(const std::string &name, const CommandFactory &factory);
    Command *createCommand(const std::string &name);
};