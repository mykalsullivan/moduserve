//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

// Forward declaration(s)
class ServerModule;

class ServerModuleManager {
    std::unordered_map<std::string, std::unique_ptr<ServerModule>> m_Subsystems;
    std::mutex m_Mutex;

public:
    template <typename Module> void registerModule();
    template <typename Module> bool hasModule();
    template <typename Module> Module *module();
    template <typename Module> Module *operator[]();
};