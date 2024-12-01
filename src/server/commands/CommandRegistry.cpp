//
// Created by msullivan on 12/1/24.
//

#include "CommandRegistry.h"

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