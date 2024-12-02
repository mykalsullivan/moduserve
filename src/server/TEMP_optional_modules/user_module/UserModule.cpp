//
// Created by msullivan on 11/10/24.
//

#include "UserModule.h"
#include "UserAuthenticationSubsystem.h"

int UserModule::init()
{
    return 0;
}

UserModule::~UserModule()
{
    for (auto &pair : m_Users) delete pair.second;
}

bool UserModule::add(int socketID, User *user)
{
    if (m_Users.find(socketID) == m_Users.end())
    {
        m_Users[socketID] = user;
        return true;
    }
    return false; // User already exists
}

bool UserModule::remove(int socketID)
{
    auto it = m_Users.find(socketID);
    if (it == m_Users.end())
        return false; // User not found
    delete it->second;  // Clean up the user
    m_Users.erase(it);
    return true;
}

User *UserModule::get(int socketFD)
{
    auto it = m_Users.find(socketFD);
    return (it != m_Users.end()) ? it->second : nullptr;
}

User *UserModule::operator[](int socketID)
{
    return get(socketID);
}

// Make this work with an individual connection
//bool UserModule::authenticateConnection(int connectionFD, const std::string &username, const std::string &password)
//{
//    return m_UserAuthenticator.authenticate(username, password);
//}
