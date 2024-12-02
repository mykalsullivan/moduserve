//
// Created by msullivan on 12/1/24.
//

#include "ServerModuleManager.h"
#include "common/PCH.h"

template <typename Module>
void ServerModuleManager::registerModule()
{
    const std::string &moduleName = Module::name();

    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    if (!m_Subsystems.contains(moduleName))
    {
        // Add subsystem to the registry
        m_Subsystems[moduleName] = std::make_unique<Module>();
        return;
    }
    throw std::runtime_error("Subsystem with name '" + moduleName + "' is already registered.");
}

template<typename Module>
[[nodiscard]] bool ServerModuleManager::hasModule()
{

    return false;
}


template <typename Module>
[[nodiscard]] Module *ServerModuleManager::module()
{
    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    auto it = m_Subsystems.find(typeid(Module));
    if (it != m_Subsystems.end())
        return it->second.get();
    throw std::runtime_error("Subsystem with name \"" + std::string(typeid(Module) + "\" not found."));
}

template <typename Module>
[[nodiscard]] Module *ServerModuleManager::operator[]()
{
    return module<Module>();
}