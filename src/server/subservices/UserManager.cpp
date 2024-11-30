//
// Created by msullivan on 11/10/24.
//

#include "UserManager.h"
#include "UserAuthenticator.h"
#include "../../User.h"

UserManager::UserManager(ConnectionManager &connectionManager) :
                        m_ConnectionManager(connectionManager)
{}

UserManager::~UserManager()
{
    for (auto &pair : m_Users) delete pair.second;
}

bool UserManager::add(int socketID, User *user)
{
    if (m_Users.find(socketID) == m_Users.end())
    {
        m_Users[socketID] = user;
        return true;
    }
    return false; // User already exists
}

bool UserManager::remove(int socketID)
{
    auto it = m_Users.find(socketID);
    if (it == m_Users.end())
        return false; // User not found
    delete it->second;  // Clean up the user
    m_Users.erase(it);
    return true;
}

User *UserManager::get(int socketFD)
{
    auto it = m_Users.find(socketFD);
    return (it != m_Users.end()) ? it->second : nullptr;
}

User *UserManager::operator[](int socketID)
{
    return get(socketID);
}

// Make this work with an individual connection
bool UserManager::authenticateConnection(int connectionFD, const std::string &username, const std::string &password)
{
    return m_UserAuthenticator.authenticate(username, password);
}
