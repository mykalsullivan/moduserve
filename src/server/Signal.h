//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <vector>
#include <mutex>
#include <functional>

template<typename... Args>
class Signal {
    std::mutex m_Mutex;
    std::vector<std::function<void(Args...)>> m_Slots;

public:
    // Connect a slot to a signal
    template <typename Slot>
    void connect(Slot&& slot)
    {
        std::lock_guard lock(m_Mutex);
        m_Slots.emplace_back(std::forward<Slot>(slot));
    }

    // Disconnect a specific slot
    template <typename Slot>
    void disconnect(Slot&& slot) {
        std::lock_guard lock(m_Mutex);
        m_Slots.erase(std::remove_if(m_Slots.begin(), m_Slots.end(),
            [&](const std::function<void()>& storedSlot) {
                return storedSlot.target_type() == typeid(slot);
            }), m_Slots.end());
    }

    // Emit the signal (invoke all connected slots)
    void emit(Args &&... args) {
        std::lock_guard lock(m_Mutex);
        for (auto& slot : m_Slots) {
            slot(std::forward<Args>(args)...);
        }
    }

    // Operator() to emit the signal
    void operator()(Args &&... args) {
        emit(std::forward<Args>(args)...);
    }
};

// Macros
#define signals
#define slots