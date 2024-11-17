//
// Created by msullivan on 11/10/24.
//

#pragma once
#include <string>
#include <queue>
#include <thread>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    Logger();
    ~Logger();

private:
    bool m_Running;
    std::queue<std::string> m_LogQueue;
    std::thread m_LogThread;
    std::mutex m_LogQueueMutex;

    [[nodiscard]] std::string getCurrentTimestamp() const;
    [[nodiscard]] std::string logLevelToString(LogLevel level) const;

public:
    static Logger &instance();
    void logMessage(LogLevel level, const std::string &message);

private:
    void logThreadFunction();
};