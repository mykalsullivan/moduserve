//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "../Subsystem.h"
#include <string>

// Forward declaration(s)
class Connection;

class BroadcastSubsystem : public Subsystem {
public:
    BroadcastSubsystem() = default;
    ~BroadcastSubsystem() override = default;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "BroadcastSubsystem"; }

    void broadcastMessage(const Connection &sender, const std::string &message);
};
