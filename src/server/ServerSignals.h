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
    static void connect(Signal signal, Slot&& slot)
    {
        std::lock_guard lock(m_Mutex);
        const auto key = std::type_index(typeid(signal));

        // Use std::bind to bind the member function with an instance of the class
        auto boundSlot = std::bind(std::forward<Slot>(slot), std::placeholders::_1);

        // Add the bound member function as a slot
        m_Connections[key].emplace_back([boundSlot]() {
            boundSlot(nullptr);  // Call the bound member function
        });
    }

    // Connect a slot to a signal
    template <typename Class, typename Signal, typename Slot>
    static void connect(Signal Class::*signal, Slot&& slot)
    {
        std::lock_guard lock(m_Mutex);
        auto key = std::type_index(typeid(signal));

        // Ensure signal is registered before connecting
        if (!m_Connections.contains(key))
            registerSignal(signal); // Register signal if not already registered

        // If the function is static, don't require the object instance.
        if constexpr (std::is_member_function_pointer_v<Slot>)
        {
            // Use std::bind for non-static functions (bind with the object)
            auto boundSlot = std::bind(std::forward<Slot>(slot), std::placeholders::_1);

            // Add the bound member function as a slot
            m_Connections[key].emplace_back([boundSlot]() {
                boundSlot(nullptr);  // Call the bound member function
            });
        }
        else
        {
            // Directly use static member function without binding
            auto boundSlot = std::forward<Slot>(slot);
            m_Connections[key].emplace_back([boundSlot]() {
                boundSlot(); // Call the static member function directly
            });
        }
    }

    // Disconnect a specific slot from a signal (function)
    template <typename Signal, typename Slot>
    static void disconnect(Signal signal, Slot&& slot)
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
        {
            // Return a callable that invokes all connected slots
            return [slots = &it->second] {
                for (auto &slot : *slots)
                    slot(); // Invoke each slot
            };
        }

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
        {
            // Return a callable that invokes all connected slots
            return [slots = &it->second]() {
                for (auto &slot : *slots)
                    slot(); // Invoke each slot
            };
        }
        return {}; // Return an empty callable if signal is not found
    }
};

// Macros
#define signals
#define slots
#define emit