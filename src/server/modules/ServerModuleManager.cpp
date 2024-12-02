//
// Created by msullivan on 12/1/24.
//

#include "ServerModuleManager.h"
#include "ServerModule.h"
#include "common/PCH.h"

void ServerModuleManager::registerModule(std::unique_ptr<ServerModule> module)
{
    const std::string &moduleName = module->name();

    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    if (!m_Subsystems.contains(moduleName))
    {
        // Add subsystem to the registry
        m_Subsystems[moduleName] = std::move(module);
        return;
    }
    throw std::runtime_error("Subsystem with name '" + moduleName + "' is already registered.");
}

ServerModule *ServerModuleManager::module(const std::string &name)
{
    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    auto it = m_Subsystems.find(name);
    if (it != m_Subsystems.end())
        return it->second.get();
    throw std::runtime_error("Subsystem with name \"" + name + "\" not found.");
}

ServerModule *ServerModuleManager::operator[](const std::string &name)
{
    return module(name);
}