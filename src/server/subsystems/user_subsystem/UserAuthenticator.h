//
// Created by msullivan on 11/11/24.
//

#pragma once
#include "../Subsystem.h"
#include <string>
#include <pqxx/pqxx>

class UserAuthenticator : public Subsystem {
public:
    UserAuthenticator();
    ~UserAuthenticator() override;

private:
    pqxx::connection *m_DatabaseConnection;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "UserAuthenticator"; }

    bool sync();
    [[nodiscard]] int registerUser(const std::string &username, const std::string &password);
    [[nodiscard]] bool authenticate(const std::string &username, const std::string &password);
    [[nodiscard]] bool usernameExists(const std::string &username) const;

private:
    std::string hashPassword(const std::string &password);
};