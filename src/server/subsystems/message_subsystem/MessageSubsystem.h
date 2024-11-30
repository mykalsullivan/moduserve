//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Subsystem.h"
#include <string>

// Forward declaration(s)
class Connection;
class Message;

class MessageSubsystem : public Subsystem {
public:
    MessageSubsystem() = default;
    ~MessageSubsystem() override = default;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "MessageSubsystem"; }

    void handleMessage(Connection &sender, const std::string &message);

private:
    void parseMessage(Connection &sender, const std::string &message) const;
};
