//
// Created by msullivan on 11/11/24.
//

#pragma once
#include <string>
#include <pqxx/pqxx>
#include <barrier>

// Forward declaration(s)
class Server;

class UserAuthenticator {
public:
    explicit UserAuthenticator(std::barrier<> &serviceBarrier);
    ~UserAuthenticator();

private:
    pqxx::connection *m_DatabaseConnection;

public:
    bool sync();
    [[nodiscard]] int registerUser(const std::string &username, const std::string &password);
    [[nodiscard]] bool authenticate(const std::string &username, const std::string &password);
    [[nodiscard]] bool usernameExists(const std::string &username) const;

private:
    std::string hashPassword(const std::string &password);
};