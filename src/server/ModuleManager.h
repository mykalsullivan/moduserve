//
// Created by msullivan on 12/4/24.
//

#pragma once
#include "modules/ServerModule.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <typeinfo>
#include <optional>

class ModuleManager {
    std::unordered_map<std::type_index, std::shared_ptr<ServerModule>> m_Modules;
    std::mutex m_Mutex;

public:
    static ModuleManager &instance()
    {
        static ModuleManager instance;
        return instance;
    }

    template<typename T, typename... Args>
    void registerModule(Args &&... args)
    {
        std::lock_guard lock(m_Mutex);
        const auto type = std::type_index(typeid(T));

        if (m_Modules.contains(type))
            throw std::runtime_error("Module of this type is already registered");
        m_Modules[type] = std::make_shared<T>(std::forward<Args>(args)...);
    }

    // Retrieve a module by type
    template<typename T>
    std::shared_ptr<T> getModule()
    {
        std::lock_guard lock(m_Mutex);
        const auto type = std::type_index(typeid(T));
        return m_Modules.contains(type) ? std::dynamic_pointer_cast<T>(m_Modules[type]) : nullptr;
    }

    // Retrieve a module by type (optional_modules, for optional_modules dependencies)
    template<typename T>
    std::optional<std::shared_ptr<T>> getOptionalModule()
    {
        auto module = getModule<T>();
        return module ? module : std::nullopt;
    }

    // Returns true if the module is loaded
    template<typename T>
    bool hasModule()
    {
        return getModule<T>() ? true : false;
    }

    // Initialize all registered modules
    void initializeModules()
    {
        std::lock_guard lock(m_Mutex);
        for (auto &[type, module] : m_Modules)
        {
            auto serverModule = std::dynamic_pointer_cast<ServerModule>(module);
            serverModule->init();
        }
    }

    // Start all registered and initialized background service modules
    void startBackgroundServices()
    {
        std::lock_guard lock(m_Mutex);
        for (auto &[type, module] : m_Modules)
        {
            auto backgroundService = std::dynamic_pointer_cast<ServerBackgroundService>(module);
            if (backgroundService && backgroundService->isInitialized())
                backgroundService->run();
        }
    }
};
