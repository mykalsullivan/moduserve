//
// Created by msullivan on 11/29/24.
//

#pragma once
#include "../ServerCommand.h"

class StopCommand : public ServerCommand {
public:
    StopCommand();
    ~StopCommand() override = default;
    void execute(const std::string &args) override;
};