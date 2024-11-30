//
// Created by msullivan on 11/9/24.
//

#include "Message.h"
#include <sstream>
#include <iomanip>
#include <utility>

Message::Message(int senderID, std::string content)
        : m_SenderID(senderID), m_Content(std::move(content)), m_Timestamp(std::chrono::system_clock::now())
{}

std::string Message::getTimestamp() const
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
    ss << "[SenderID: " << m_SenderID << "] ";
    ss << "[Timestamp: " << getTimestamp() << "] ";
    ss << "[Content: " << m_Content << "]";
    return ss.str();
}