//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <unordered_map>
#include <mutex>
#include <any>

class SignalManager {
    std::mutex m_Mutex;
    std::unordered_map<std::string, std::any> m_Signals;
public:
    template<typename SignalType>
    void registerSignal(const std::string& name, SignalType& signal)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Signals[name] = &signal;
    }

    template<typename SignalType>
    SignalType *get(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Signals.find(name);
        return it != m_Signals.end() ? std::any_cast<SignalType *>(it->second) : nullptr;
    }
};


