//
// Created by msullivan on 12/1/24.
//

#include "SubsystemManager.h"
#include "Subsystem.h"
#include "common/PCH.h"

void SubsystemManager::registerSubsystem(std::unique_ptr<ServerModule> subsystem)
{
    const std::string &subsystemName = subsystem->name();

    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    if (!m_Subsystems.contains(subsystemName))
    {
        // Add subsystem to the registry
        m_Subsystems[subsystemName] = std::move(subsystem);
        return;
    }
    throw std::runtime_error("Subsystem with name '" + subsystemName + "' is already registered.");
}

ServerModule *SubsystemManager::subsystem(const std::string &name)
{
    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    auto it = m_Subsystems.find(name);
    if (it != m_Subsystems.end())
        return it->second.get();
    throw std::runtime_error("Subsystem with name \"" + name + "\" not found.");
}

ServerModule *SubsystemManager::operator[](const std::string &name)
{
    return subsystem(name);
}