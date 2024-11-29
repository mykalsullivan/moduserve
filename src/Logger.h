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

class Logger {
public:
    Logger() = default;
    ~Logger() = default;

private:

    [[nodiscard]] std::string getCurrentTimestamp() const;
    [[nodiscard]] std::string logLevelToString(LogLevel level) const;

public:
    static Logger &instance();
    void logMessage(LogLevel level, const std::string &message);
};

#define LOG(logLevel, message) Logger::instance().logMessage(logLevel, message);