//
// Created by msullivan on 11/11/24.
//

#pragma once

#include "Server.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <pqxx/pqxx>

class UserAuthenticator {
public:
    explicit UserAuthenticator(Server &server);
    ~UserAuthenticator();

private:
    Server &m_Server;

    pqxx::connection *m_DatabaseConnection;

public:
    bool sync();
    int registerUser(const std::string &username, const std::string &password);
    bool authenticate(const std::string &username, const std::string &password);
    bool usernameExists(const std::string &username) const;

private:
    std::string hashPassword(const std::string &password);
};