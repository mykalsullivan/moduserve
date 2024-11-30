//
// Created by msullivan on 11/10/24.
//

#include "UserManager.h"
#include "UserAuthenticator.h"
#include "../User.h"

UserManager::UserManager(ConnectionManager &connectionManager,
                        UserAuthenticator &userAuthenticator,
                        std::barrier<> &serviceBarrier) :
                        m_ConnectionManager(connectionManager),
                        m_UserAuthenticator(userAuthenticator)
{
    // Wait for all services to be initialized
    serviceBarrier.arrive_and_wait();
}

UserManager::~UserManager()
{
    std::lock_guard lock(m_UserMutex);

    // Clean up
    for (auto &pair : m_Users) delete pair.second;
}

bool UserManager::addUser(int socketID, User *user)
{
    std::lock_guard lock(m_UserMutex);

    if (m_Users.find(socketID) == m_Users.end())
    {
        m_Users[socketID] = user;
        return true;
    }
    return false; // User already exists
}

bool UserManager::removeUser(int socketID)
{
    std::lock_guard lock(m_UserMutex);

    auto it = m_Users.find(socketID);
    if (it == m_Users.end())
        return false; // User not found
    delete it->second;  // Clean up the user
    m_Users.erase(it);
    return true;
}

User *UserManager::getUser(int socketFD)
{
    std::lock_guard lock(m_UserMutex);
    auto it = m_Users.find(socketFD);
    return (it != m_Users.end()) ? it->second : nullptr;
}

User *UserManager::operator[](int socketID)
{
    return getUser(socketID);
}

// Make this work with an individual connection
bool UserManager::authenticateConnection(int connectionFD, const std::string &username, const std::string &password)
{
    std::lock_guard lock(m_UserMutex);
    return m_UserAuthenticator.authenticate(username, password);
}
