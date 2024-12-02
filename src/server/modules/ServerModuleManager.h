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
    auto begin() const { return m_Subsystems.begin(); }
    auto end() const { return m_Subsystems.end(); }
    auto find(const std::string &name) const { return m_Subsystems.find(name); }
    [[nodiscard]] bool contains(const std::string &name) const { return m_Subsystems.contains(name); }
    [[nodiscard]] size_t size() const { return m_Subsystems.size(); }
    
    void registerModule(std::unique_ptr<ServerModule> module);
    ServerModule *module(const std::string &name);
    ServerModule *operator[](const std::string &name);
};