//
// Created by msullivan on 11/30/24.
//

#pragma once
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename... Args>
class Signal {
    using Slot = std::function<void(Args...)>;
public:
    Signal() = default;
    ~Signal() = default;

private:
    std::vector<std::shared_ptr<Slot>> m_Slots;
    mutable std::mutex m_Mutex;

public:
    std::shared_ptr<Slot> connect(Slot slot)
    {
        auto slotPtr = std::make_shared<Slot>(std::move(slot));
        {
            std::unique_lock lock(m_Mutex);
            m_Slots.emplace_back(slot);
        }
        return slotPtr; // Return the pointer to the slot
    }

    void disconnect(const std::shared_ptr<Slot> &slot)
    {
        std::unique_lock lock(m_Mutex);

        auto it = std::remove(m_Slots.begin(), m_Slots.end(), slot);

        m_Slots.erase(
            std::remove_if(
                m_Slots.begin(), m_Slots.end(),
                [&slot](const std::shared_ptr<Slot>& storedSlot) { return storedSlot == slot; }
            ),
            m_Slots.end()
        );
    }

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
#define connectTo(signal, emitter, slot) (signal).connect(emitter, slot)
#define emit(signal, ...) (signal).emit(__VA_ARGS__)