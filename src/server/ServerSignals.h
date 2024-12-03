//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <typeindex>

class ServerSignal {
    static std::mutex m_Mutex;
    static std::unordered_map<std::type_index, std::vector<std::function<void()>>> m_Connections;
public:
    template<typename Signal>
    static void registerSignal(Signal signal)
    {
        std::lock_guard lock(m_Mutex);
        // Initialize empty slot list if not already present
        if (const auto key = std::type_index(typeid(signal)); !m_Connections.contains(key))
            m_Connections[key] = {};
    }

    template<typename Class, typename Signal>
    static void registerSignal(Signal Class::*func)
    {
        std::lock_guard lock(m_Mutex);

        // Initialize empty slot list if not already present
        if (const auto key = std::type_index(typeid(func)); !m_Connections.contains(key))
            m_Connections[key] = {};
    }

    // Connect a slot to a signal
    template <typename Signal, typename Slot>
    static void connect(Signal &&signal, Slot &&slot)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        // Ensure the signal is registered
        if (!m_Connections.contains(key))
            registerSignal(signal);

        // Store the slot as a lambda
        m_Connections[key].emplace_back([slot = std::forward<Slot>(slot)](auto &&... args) mutable {
            std::invoke(slot, std::forward<decltype(args)>(args)...);  // Invoke function
        });
    }

    // Connect a slot to a signal
    template <typename Class, typename Func, typename Obj>
    static void connect(Func Class::*signal, Obj &&obj, Func &&slot)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        // Ensure the signal is registered
        if (!m_Connections.contains(key))
            registerSignal(signal);

        // Handle static functions or callable objects
        m_Connections[key].emplace_back([obj = std::forward<Obj>(obj), slot](auto &&...args) mutable {
            std::invoke(slot, obj, std::forward<decltype(args)>(args)...);  // Deferred invocation
        });
    }

    // Disconnect a specific slot from a signal (function)
    template <typename Signal, typename Slot>
    static void disconnect(Signal signal, Slot &&slot)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        const auto it = m_Connections.find(key);
        if (!m_Connections.contains(key))
        {
            // Remove the slot using comparison
            auto& slots = it->second;
            slots.erase(std::remove_if(slots.begin(), slots.end(),
                                       [&](const std::function<void()>& s) {
                                           return s.target_type() == typeid(slot);
                                       }),
                        slots.end());

            // If no slots left, unregister the signal
            if (slots.empty()) m_Connections.erase(it);
        }
    }

    // Disconnect a specific slot from a signal (class method)
    template <typename Class, typename Signal, typename Slot>
    static void disconnect(Signal Class::*signal, Slot&& slot)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        if (auto it = m_Connections.find(key); it != m_Connections.end())
        {
            // Remove the slot using comparison
            auto& slots = it->second;
            slots.erase(std::remove_if(slots.begin(), slots.end(),
                                       [&](const std::function<void()>& s) {
                                           return s.target_type() == typeid(slot);
                                       }),
                        slots.end());

            // If no slots left, unregister the signal
            if (slots.empty())
                m_Connections.erase(it);
        }
    }

    // Retrieves a callable from the signal (function)
    template<typename Signal>
    static std::function<void()> get(Signal signal)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        const auto it = m_Connections.find(key);
        if (it != m_Connections.end())
            // Return a callable that invokes all connected slots
            return [slots = &it->second] {
                for (auto &slot : *slots)
                    slot(); // Invoke each slot
            };
        return {}; // Return an empty callable if signal is not found
    }

    // Retrieves a callable from the signal (class method)
    template<typename Class, typename Signal>
    static std::function<void()> get(Signal Class::*signal)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        const auto it = m_Connections.find(key);
        if (it != m_Connections.end())
            // Return a callable that invokes all connected slots
            return [slots = &it->second]() {
                for (auto &slot : *slots)
                    slot(); // Invoke each slot
            };
        return {}; // Return an empty callable if signal is not found
    }

    // Emit a signal to invoke all connected slots
    template<typename Signal, typename... Args>
    static void emit(Signal &&signal, Args &&...args) {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        const auto it = m_Connections.find(key);
        if (it != m_Connections.end())
            for (const auto& slot : it->second)
                slot(std::forward<Args>(args)...);  // Invoke each connected slot
    }

    // Emit a class signal to invoke all connected slots
    template<typename Class, typename Signal, typename... Args>
    static void emit(Signal Class::*signal, Args&&... args)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        const auto it = m_Connections.find(key);
        if (it != m_Connections.end())
            for (const auto& slot : it->second)
                slot(std::forward<Args>(args)...);  // Invoke each connected slot
    }
};

// Static members
std::mutex ServerSignal::m_Mutex;
std::unordered_map<std::type_index, std::vector<std::function<void()>>> ServerSignal::m_Connections;

// Macros
#define signals
#define slots