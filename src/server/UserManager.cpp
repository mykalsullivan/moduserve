//
// Created by msullivan on 11/10/24.
//

#include "UserManager.h"
#include "Server.h"
#include "UserAuthenticator.h"

UserManager::UserManager(Server &server) : m_Server(server)
{}

UserManager::~UserManager()
{
    std::lock_guard lock(m_UserMutex);

    // Clean up
    for (auto &pair : m_Users)
        delete pair.second;
}

bool UserManager::addUser(int socketID, User *user)
{
    std::lock_guard lock(m_UserMutex);

    if (m_Users.find(socketID) != m_Users.end()) {
        return false; // User already exists
    }
    m_Users[socketID] = user;
    return true;
}

bool UserManager::removeUser(int socketID)
{
    std::lock_guard lock(m_UserMutex);

    auto it = m_Users.find(socketID);
    if (it == m_Users.end()) {
        return false; // User not found
    }
    delete it->second;  // Clean up the user
    m_Users.erase(it);
    return true;
}

User *UserManager::getUser(int socketID)
{
    std::lock_guard lock(m_UserMutex);

    auto it = m_Users.find(socketID);
    if (it != m_Users.end()) {
        return it->second;
    }
    return nullptr;
}

User *UserManager::operator[](int socketID)
{
    std::lock_guard lock(m_UserMutex);

    auto it = m_Users.find(socketID);
    if (it != m_Users.end()) {
        return it->second;
    }
    return nullptr;
}

size_t UserManager::size() const
{
    std::lock_guard lock(m_UserMutex);
    return m_Users.size();
}

bool UserManager::empty() const
{
    std::lock_guard lock(m_UserMutex);
    return m_Users.empty();
}

auto UserManager::begin()
{
    std::lock_guard lock(m_UserMutex);
    return m_Users.begin();
}

auto UserManager::end()
{
    std::lock_guard lock(m_UserMutex);
    return m_Users.end();
}

// Make this work with an individual connection
bool UserManager::authenticateConnection(int connectionID, const std::string &username, const std::string &password)
{
    std::lock_guard lock(m_UserMutex);
    return m_Server.m_UserAuthenticator.authenticate(username, password);
}
