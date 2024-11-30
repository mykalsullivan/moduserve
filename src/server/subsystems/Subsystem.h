//
// Created by msullivan on 11/30/24.
//

#pragma once
#include <string>

class Subsystem {
public:
    virtual ~Subsystem() = default;
    [[nodiscard]] virtual int init() = 0;
    [[nodiscard]] virtual constexpr std::string name() const = 0;
};