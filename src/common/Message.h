//
// Created by msullivan on 11/9/24.
//

#pragma once
#include <string>
#include <chrono>

class Message {
public:
    [[maybe_unused]] Message(std::string ip, unsigned int port, std::string body);

private:
    const std::string m_SenderIP;
    const unsigned int m_SenderPort;
    std::string m_Content;
    std::chrono::system_clock::time_point m_Timestamp;

public:
    [[nodiscard]] std::string ip() const { return m_SenderIP; }
    [[nodiscard]] unsigned int port() const { return m_SenderPort; }
    [[nodiscard]] std::string content() const { return m_Content; }
    [[nodiscard]] std::string timestamp() const;
    [[nodiscard]] std::string toString() const;
};