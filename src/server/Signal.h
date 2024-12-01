//
// Created by msullivan on 11/30/24.
//

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename... Args>
class Signal {
    using Slot = std::function<void(Args...)>;

    std::vector<std::shared_ptr<Slot>> m_Slots;
    mutable std::shared_mutex m_Mutex;

public:
    // Thread-safe connect method with shared_mutex for concurrent reads
    template<typename Callable>
    std::shared_ptr<Slot> connect(Callable &&callable)
    {
        std::shared_ptr<Slot> slotPtr;

        // If the callable is a member function...
        if constexpr (std::is_member_function_pointer_v<std::decay_t<Callable>>)
        {
            slotPtr = std::make_shared<Slot>([this, callable](Args... args) {
                if constexpr (std::is_const_v<std::remove_reference_t<Callable>>)
                    (this->*callable)(args...); // const member function
                else
                    (this->*callable)(args...); // non-const member function
            });
        }
        else
            // Static function or lambda: Just use it directly
            slotPtr = std::make_shared<Slot>(std::forward<Callable>(callable));

        std::lock_guard lock(m_Mutex);
        m_Slots.emplace_back(slotPtr);
        return slotPtr;
    }

    // Thread-safe disconnect method
    void disconnect(const std::shared_ptr<Slot> &slot)
    {
        std::unique_lock lock(m_Mutex);
        auto it = std::remove(m_Slots.begin(), m_Slots.end(), slot);
        m_Slots.erase(it, m_Slots.end());
    }

    // Emit the signal to all connected slots
    void emit(Args... args)
    {
        std::vector<std::shared_ptr<Slot>> slotsCopy;
        {
            std::shared_lock lock(m_Mutex); // Shared lock for reading
            slotsCopy = m_Slots;               // Make a copy of the current slots
        }
        for (const auto& slot : m_Slots)
            if (slot && *slot)
                (*slot)(args...);   // Invoke the slot
    }
};

#define slots
#define signals