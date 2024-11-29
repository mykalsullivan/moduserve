//
// Created by msullivan on 11/11/24.
//

#pragma once
#include <string>
#include <pqxx/pqxx>

// Forward declaration(s)
class Server;

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