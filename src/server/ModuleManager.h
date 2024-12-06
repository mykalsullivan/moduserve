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

#include "server/modules/Logger.h"

// static void onModuleInitialized(const std::string &moduleName)
// {
//     std::string message = "Module \"" + std::string(typeid(t).name()) + "\" initialized";
//     Logger::log(LogLevel::Info, message);
// }
//
// static void onModuleStarted(const std::string &moduleName)
// {
//     std::string message = "Module \"" + std::string(typeid(t).name()) + "\" started";
//     Logger::log(LogLevel::Info, message);
// }
//
// static void onModuleStopped(const std::string &moduleName)
// {
//     std::string message = "Module \"" + std::string(typeid(t).name()) + "\" stopped";
//     Logger::log(LogLevel::Info, message);
// }
//
// static void onModuleCrashed(const std::string &moduleName)
// {
//     std::string message = "Module \"" + moduleName + "\" crashed";
//     Logger::log(LogLevel::Error, message);
// }

class ModuleManager {
    std::unordered_map<std::type_index, std::shared_ptr<ServerModule>> m_Modules;
    std::mutex m_Mutex;

public:
    static ModuleManager &instance()
    {
        static ModuleManager instance;
        return instance;
    }

    template <typename T, typename... Args>
    void registerModule(Args &&... args)
    {
        std::lock_guard lock(m_Mutex);
        const auto type = std::type_index(typeid(T));

        if (m_Modules.contains(type))
            throw std::runtime_error("Module of this type is already registered.");
        m_Modules[type] = std::make_shared<T>(std::forward<Args>(args)...);

        // // Connect signals to slots
        // std::string moduleName = std::string(typeid(T).name());
        //
        // m_Modules[type]->initialized.connect(onModuleInitialized);
        // m_Modules[type]->started.connect(onModuleStarted);
        // m_Modules[type]->stopped.connect(onModuleStopped);
        // m_Modules[type]->crashed.connect(onModuleCrashed);
    }

    // Retrieve a module by type
    template <typename T>
    std::shared_ptr<T> getModule()
    {
        std::lock_guard lock(m_Mutex);
        const auto type = std::type_index(typeid(T));
        return m_Modules.contains(type) ? std::dynamic_pointer_cast<T>(m_Modules[type]) : nullptr;
    }

    // Retrieve a module by type (optional_modules, for optional_modules dependencies)
    template <typename T>
    std::optional<std::shared_ptr<T>> getOptionalModule()
    {
        auto module = getModule<T>();
        return module ? module : std::nullopt;
    }

    // Initialize all registered modules
    void initializeModules()
    {
        std::lock_guard lock(m_Mutex);
        for (auto &[type, module] : m_Modules)
            module->init();
    }

    // Start all registered and initialized modules
    void startModules()
    {
        std::lock_guard lock(m_Mutex);
        for (auto &[type, module] : m_Modules)
            module->run();
    }
};
