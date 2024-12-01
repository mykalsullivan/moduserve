//
// Created by msullivan on 11/30/24.
//

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <mutex>

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    std::vector<Slot> m_Slots;
    mutable std::mutex m_Mutex;

public:
    // Emit the signal to all connected slots
    void emit(Args... args)
    {
        std::vector<Slot> slotsCopy;
        {
            std::lock_guard lock(m_Mutex); // Lock for reading
            slotsCopy = m_Slots;               // Make a copy of the current slots
        }
        // Invoke each slot
        for (const auto& slot : slotsCopy) slot(args...);
    }

    const std::vector<Slot> &getSlots() const { return m_Slots; }
};

template <typename... Args, typename SlotFunc>
void connectSignal(Signal<Args...> &signal, SlotFunc &&slot)
{
    typename Signal<Args...>::Slot slotFunc = std::forward<SlotFunc>(slot);
    std::lock_guard lock(signal.m_Mutex);
    signal.m_Slots.emplace_back(std::move(slotFunc));  // Add the static function or lambda slot
}

// Thread-safe connectSignal method with shared_mutex for concurrent reads
template<typename... Args, typename SlotFunc>
void connectSignal(Signal<Args...> &signal, void *receiver, SlotFunc &&slot)
{
    typename Signal<Args...>::Slot slotFunc;

    // If the slotFunc is a member function...
    if constexpr (std::is_member_function_pointer_v<std::decay_t<SlotFunc>>)
    {
        // Handle const member functions
        if constexpr (std::is_const_v<std::remove_reference_t<SlotFunc>>)

            slotFunc = [receiver, slot](Args... args) {
                auto obj = static_cast<typename std::remove_pointer_t<std::decay<SlotFunc> *>>(receiver); // Get the const object
                (obj->*slot)(args...);  // Call const member function
            };
            // Handle non-const member functions
        else
            slotFunc = [receiver, slot](Args... args) {
                auto obj = static_cast<typename std::remove_pointer_t<std::decay<SlotFunc> *>>(receiver); // Get the const object
                (obj->*slot)(args...);  // Call non-const member function
            };
    }
    else
        // Static function or lambda: Just use it directly
        slotFunc = std::forward<SlotFunc>(slotFunc);

    std::lock_guard lock(signal.m_Mutex);
    signal.m_Slots.emplace_back(std::move(slotFunc));
}

// Thread-safe disconnectSignal method
template <typename... Args>
void disconnectSignal(Signal<Args...> &signal, typename Signal<Args...>::Slot &slot)
{
    std::lock_guard lock(signal.m_Mutex);
    auto it = std::remove(signal.m_Slots.begin(), signal.m_Slots.end(), slot);
    signal.m_Slots.erase(it, signal.m_Slots.end());
}

template <typename... Args>
void disconnectSignal(Signal<Args...>& signal, typename Signal<Args...>::Slot* slot)
{
    std::lock_guard lock(signal.m_Mutex);

    // Remove the slot from the list of slots
    auto it = std::remove_if(
    signal.m_Slots.begin(), signal.m_Slots.end(),
    [slot](const typename Signal<Args...>::Slot& slotRef) {
        return &slotRef == slot;  // Compare pointers
    });
    signal.m_Slots.erase(it, signal.m_Slots.end());
}

#define slots
#define signals

#define SIGNAL(signalName, ...) Signal<__VA_ARGS__> signalName
#define SLOT(slotName, returnType, ...) static returnType slotName(__VA_ARGS__)

#define REGISTER_SIGNAL(signalName, signal) server.signalManager().registerSignal(signalName, signal)
#define GET_SIGNAL(signalName, ...) server.signalManager().get<Signal<__VA_ARGS__>>(signalName)
