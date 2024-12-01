//
// Created by msullivan on 11/10/24.
//

#pragma once
#include <string>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

void logMessage(LogLevel level, const std::string &message);