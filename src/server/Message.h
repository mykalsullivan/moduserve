//
// Created by msullivan on 11/9/24.
//

#pragma once
#include <string>
#include <chrono>

class Message {
public:
    Message(int senderID, std::string body);

private:
    int m_SenderID;
    std::string m_Content;
    std::chrono::system_clock::time_point m_Timestamp;

public:
    [[nodiscard]] int getSenderID() const { return m_SenderID; }
    [[nodiscard]] std::string getContent() const { return m_Content; }
    [[nodiscard]] std::string getTimestamp() const;
    [[nodiscard]] std::string toString() const;
};