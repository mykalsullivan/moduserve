//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "../Command.h"

class HelpCommand : public Command {
public:
    ~HelpCommand() override = default;
    void execute(const std::string &args) override;
    [[nodiscard]] constexpr std::string name() const override { return "help"; }
    [[nodiscard]] std::string usage() const override;
};