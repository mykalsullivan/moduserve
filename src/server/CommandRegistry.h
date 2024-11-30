//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <barrier>

// Forward declaration(s)
class Command;

class CommandRegistry {
    using CommandFactory = std::function<Command *()>;
public:
    explicit CommandRegistry(std::barrier<> &serviceBarrier);
    ~CommandRegistry() = default;

private:
    std::unordered_map<std::string, CommandFactory> m_CommandFactories;

public:
    auto begin() const { return m_CommandFactories.begin(); }
    auto end() const { return m_CommandFactories.end(); }
    auto find(const std::string &commandName) const { return m_CommandFactories.find(commandName); }
    [[nodiscard]] bool contains(const std::string &commandName) const { return m_CommandFactories.contains(commandName); }
    [[nodiscard]] bool empty() const { return m_CommandFactories.empty(); }
    [[nodiscard]] size_t size() const { return m_CommandFactories.size(); }

    void loadCommand(const std::string &libPath);
    void registerCommand(const std::string &name, const CommandFactory &factory);
    Command *createCommand(const std::string &name);
};

