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
    ~MessageSubsystem() override = default;

    Signal<> beforeParse;
    Signal<> afterParse;
    Signal<const Connection &, const std::string &> onReceive;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "MessageSubsystem"; }

private:
    static void handleMessage(const Connection &sender, const std::string &message);
    static void parseMessage(const Connection &sender, const std::string &message);
};
