//
// Created by msullivan on 11/9/24.
//

#include "Message.h"
#include <iomanip>
#include <utility>

// Will eventually need to have JSON formatting

[[maybe_unused]] Message::Message(std::string ip, unsigned int port, std::string content)
        : m_SenderIP(std::move(ip)), m_SenderPort(port), m_Content(std::move(content)), m_Timestamp(std::chrono::system_clock::now())
{}

std::string Message::timestamp() const
{
    auto time = std::chrono::system_clock::to_time_t(m_Timestamp);  // Convert to time_t
    std::tm* tm = std::localtime(&time);                             // Convert to local time

    std::stringstream ss;
    ss << std::put_time(tm, "%a %b %d %H:%M:%S %Y");  // Format timestamp as a string
    return ss.str();
}

std::string Message::toString() const
{
    std::stringstream ss;
    ss << "[SenderIP: " << m_SenderIP << "] ";
    ss << "[SenderPort: " << m_SenderPort << "] ";
    ss << "[Timestamp: " << timestamp() << "] ";
    ss << "[Content: " << m_Content << "]";
    return ss.str();
}