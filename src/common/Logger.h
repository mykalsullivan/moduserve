//
// Created by msullivan on 11/10/24.
//

#pragma once
#include <string>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

void logMessage(LogLevel level, const std::string &message);