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

    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "MessageSubsystem"; }

    void handleMessage(const Connection &sender, const std::string &message);

private:
    void parseMessage(const Connection &sender, const std::string &message) const;
};
