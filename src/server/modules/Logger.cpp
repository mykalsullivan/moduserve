//
// Created by msullivan on 12/4/24.
//

#include "Logger.h"
#include <iomanip>

// Forward declaration(s)
std::string getCurrentTimestamp();
std::string logLevelToString(LogLevel);

Logger::~Logger() = default;

void Logger::init() {}
void Logger::run() {}

void Logger::log(LogLevel level, const std::string &message)
{
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = logLevelToString(level);

    std::string colorCode;
    switch (level) {
        case LogLevel::Debug:
            colorCode = "\033[36m"; // Cyan
        break;
        case LogLevel::Info:
            colorCode = "\033[37m"; // White (default)
        break;
        case LogLevel::Warning:
            colorCode = "\033[33m"; // Yellow
        break;
        case LogLevel::Error:
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

std::string getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto timePoint = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&timePoint);

    std::ostringstream timestampStream;
    timestampStream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return timestampStream.str();
}

std::string logLevelToString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}
