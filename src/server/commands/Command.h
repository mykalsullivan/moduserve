//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <string>

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(const std::string &args) = 0;
    [[nodiscard]] constexpr virtual std::string name() const = 0;
    [[nodiscard]] constexpr virtual std::string usage() const = 0;
};