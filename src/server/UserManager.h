//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../User.h"
#include <string>
#include <unordered_map>
#include <mutex>

// Forward declaration(s)
class Server;

class UserManager {
public:
    explicit UserManager(Server &server);
    ~UserManager();

private:
    Server &m_Server;
    std::unordered_map<int, User *> m_Users;
    mutable std::mutex m_UserMutex;

public:
    bool addUser(int socketID, User *user);
    bool removeUser(int connectionID);
    User *getUser(int connectionID);
    User *operator[](int connectionID);

    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] auto begin();
    [[nodiscard]] auto end();

    [[nodiscard]] bool authenticateConnection(int connectionID, const std::string &username, const std::string &password);
};