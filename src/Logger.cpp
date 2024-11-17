//
// Created by msullivan on 11/10/24.
//

#include "Logger.h"
#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>

Logger::Logger() : m_Running(true)
{
    m_LogThread = std::thread(&Logger::logThreadFunction, this);
}

Logger::~Logger()
{
    m_Running = false;
    if (m_LogThread.joinable())
        m_LogThread.join();
}

Logger &Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::logMessage(LogLevel level = LogLevel::INFO, const std::string &message = "")
{
    std::lock_guard lock(m_LogQueueMutex);

    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = logLevelToString(level);

    // Choose color based on the log level
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

    // // Store the current line content (simulate the "copy" behavior)
    // std::string previousLineContent = logStream.str();
    //
    // // Clear the current line (remove the old log message)
    // std::cout << "\033[K";  // Clear the current line
    //
    // // Move the cursor down by 1 line
    // std::cout << "\033[1B"; // Move cursor down
    //
    // // Print the previous line's content (simulate copying the previous line)
    // std::cout << previousLineContent << std::endl;
    //
    // // Now move the cursor back up to the original line, and clear it for the new log message
    // std::cout << "\033[1A"; // Move the cursor back up by one line
    //
    // // Now print the new log message on the original line
    // std::cout << logStream.str() << std::endl;


    // Save the current user input line (i.e., ::) position
    std::cout << "\033[s";  // Save cursor position (before overwriting)

    // Clear the current input line (optional, to clean up before printing log)
    std::cout << "\033[K";  // Clear the current line where user input is

    // Print the log message, overwriting the input line
    std::cout << logStream.str();

    // Move the cursor down by one line to make space for the input prompt
    std::cout << "\033[1B";  // Move cursor down 1 line

    // Restore the cursor position (where user input should be printed)
    std::cout << "\033[u";  // Restore cursor position to saved position

    // Print the user input prompt (on the new line)
    std::cout << ":: ";

    m_LogQueue.push(logStream.str());
}

void Logger::logThreadFunction()
{
    while (m_Running)
    {
        std::lock_guard lock(m_LogQueueMutex);
        if (!m_LogQueue.empty()) {
            std::string log = m_LogQueue.front();
            m_LogQueue.pop();
            std::cout << log << std::endl;
        }
    }
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
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}