//
// Created by msullivan on 11/30/24.
//

#pragma once
#include <string>
#include "server/Signal.h"

class Subsystem {
public:
    virtual ~Subsystem() = default;
    virtual int init() = 0;
    [[nodiscard]] virtual constexpr std::string name() const = 0;
};