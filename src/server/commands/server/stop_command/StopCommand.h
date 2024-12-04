//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "server/commands/Command.h"

class StopCommand : public Command {
public:
    ~StopCommand() override = default;
    void execute(const std::string &args) override;
    [[nodiscard]] constexpr std::string name() const override { return "stop"; }
    [[nodiscard]] std::string usage() const override;
};