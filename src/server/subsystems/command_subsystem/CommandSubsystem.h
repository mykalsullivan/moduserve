//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "../Subsystem.h"

class CommandSubsystem : public Subsystem {
public:
    ~CommandSubsystem() override = default;

    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "CommandSubsystem"; }

    void loadCommand(const std::string &libPath);
};