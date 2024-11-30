//
// Created by msullivan on 11/9/24.
//

#include "User.h"
#include <utility>

User::User(std::string &username,
    std::chrono::system_clock::time_point creationDate,
    std::chrono::system_clock::time_point lastLogin,
    std::string status,
    std::string bio) :
    m_Username(username),
    m_CreationDate(creationDate),
    m_LastLogin(lastLogin),
    m_Status(std::move(status)),
    m_Bio(std::move(bio))

{}

void User::setUsername(const std::string &username)
{
    m_Username = username;
}


void User::addMessageToHistory(const std::string& message)
{
    //m_MessageHistory.push_back(message);
}