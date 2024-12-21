//
// Created by msullivan on 12/21/24.
//

#pragma once
#include "../ServerCommand.h"

class NullCommand : public ServerCommand {
public:
    ~NullCommand() override = default;
    void execute(const std::vector<std::string> &args) override;
};