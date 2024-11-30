//
// Created by msullivan on 11/10/24.
//

#include "Logger.h"
#include "PCH.h"
#include <iomanip>

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::logMessage(LogLevel level = LogLevel::INFO, const std::string &message = "")
{
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = logLevelToString(level);

    std::string colorCode;
    switch (level) {
        case LogLevel::DEBUG:
            colorCode = "\033[36m"; // Cyan
        break;
        case LogLevel::INFO:
            colorCode = "\033[37m"; // White (default)
        break;
        case LogLevel::WARNING:
            colorCode = "\033[33m"; // Yellow
        break;
        case LogLevel::ERROR:
            colorCode = "\033[31m"; // Red
        break;
        default:
            colorCode = "\033[0m";  // Reset to default
        break;
    }

    std::ostringstream logStream;
    logStream << colorCode << "[" << timestamp << "] [" << levelStr << "] " << message << "\033[0m";
    std::cout << logStream.str() << std::endl;
}

std::string Logger::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto timePoint = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timePoint);

    std::ostringstream timestampStream;
    timestampStream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return timestampStream.str();
}

std::string Logger::logLevelToString(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}