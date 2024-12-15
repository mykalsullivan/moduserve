//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "../ServerCommand.h"

class HelpCommand : public ServerCommand {
public:
    HelpCommand();
    ~HelpCommand() override = default;
    void execute(const std::string &args) override;
};