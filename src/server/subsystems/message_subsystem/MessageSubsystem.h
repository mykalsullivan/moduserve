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

public signals:
    SIGNAL(beforeParse);
    SIGNAL(afterParse);
    SIGNAL(onReceive, const Connection &, const std::string &);

private slots:
    SLOT(handleMessage, void, const Connection &sender, const std::string &message);
    SLOT(parseMessage, void, const Connection &sender, const std::string &message);

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "MessageSubsystem"; }
};
