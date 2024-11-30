//
// Created by msullivan on 11/10/24.
//

#include "UserSubsystem.h"
#include "UserAuthenticationSubsystem.h"
#include "server/User.h"

int UserSubsystem::init()
{
    return 0;
}

UserSubsystem::~UserSubsystem()
{
    for (auto &pair : m_Users) delete pair.second;
}

bool UserSubsystem::add(int socketID, User *user)
{
    if (m_Users.find(socketID) == m_Users.end())
    {
        m_Users[socketID] = user;
        return true;
    }
    return false; // User already exists
}

bool UserSubsystem::remove(int socketID)
{
    auto it = m_Users.find(socketID);
    if (it == m_Users.end())
        return false; // User not found
    delete it->second;  // Clean up the user
    m_Users.erase(it);
    return true;
}

User *UserSubsystem::get(int socketFD)
{
    auto it = m_Users.find(socketFD);
    return (it != m_Users.end()) ? it->second : nullptr;
}

User *UserSubsystem::operator[](int socketID)
{
    return get(socketID);
}

// Make this work with an individual connection
//bool UserSubsystem::authenticateConnection(int connectionFD, const std::string &username, const std::string &password)
//{
//    return m_UserAuthenticator.authenticate(username, password);
//}
