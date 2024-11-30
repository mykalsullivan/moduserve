//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "Subsystem.h"
#include <string>
#include <unordered_map>
#include <mutex>

// Forward declaration(s)
class ConnectionManager;
class User;

class UserManager : public Subsystem {
public:
    explicit UserManager(ConnectionManager &connectionManager);
    ~UserManager() override;

private:
    ConnectionManager &m_ConnectionManager;
    std::unordered_map<int, User *> m_Users;

public:
    int init() override;
    [[nodiscard]] std::string name() override { return "userManager"; }

    bool add(int socketID, User *user);
    bool remove(int connectionID);
    User *get(int connectionID);
    User *operator[](int connectionID);

    [[nodiscard]] size_t size() const { return m_Users.size(); }
    [[nodiscard]] bool empty() const { return m_Users.empty(); }
    [[nodiscard]] auto begin() { return m_Users.begin(); }
    [[nodiscard]] auto end() { return m_Users.end(); }

    [[nodiscard]] bool authenticateConnection(int connectionID, const std::string &username, const std::string &password);
};