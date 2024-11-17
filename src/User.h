//
// Created by msullivan on 11/9/24.
//

#pragma once
#include "UUID.h"
#include "Role.h"
#include <string>
#include <vector>
#include <chrono>

// Forward declaration(s)
class Connection;

class User {
public:
    User(std::string &username,
        std::chrono::system_clock::time_point creationDate,
        std::chrono::system_clock::time_point lastLogin,
        std::string status,
        std::string bio);

private:
    UUID m_UUID;
    std::string m_Username;
    std::string m_PasswordHash;
    std::chrono::system_clock::time_point m_CreationDate; // Account creation date
    std::chrono::system_clock::time_point m_LastLogin; // Last login timestamp
    std::string m_Status;
    std::string m_Bio;
    std::vector<Role> m_Roles;
    std::vector<std::string> m_MessageHistory;

public:
    void setUsername(const std::string &username);
    void setPasswordHash(const std::string &passwordHash);
    void setLastLogin(const std::chrono::system_clock::time_point &lastLogin);
    void setStatus(const std::string &status);
    void setBio(const std::string &bio);
    void addRole(const Role &role);

    [[nodiscard]] std::string getUUID() const { return m_UUID.toString(); }
    [[nodiscard]] const std::string &getUsername() const { return m_Username; };
    [[nodiscard]] const std::string &getPasswordHash() const { return m_PasswordHash; }
    [[nodiscard]] std::chrono::system_clock::time_point getCreationDate() const { return m_CreationDate; }
    [[nodiscard]] std::chrono::system_clock::time_point getLastLogin() const { return m_LastLogin; }
    [[nodiscard]] const std::string &getStatus() const { return m_Status; }
    [[nodiscard]] const std::string &getBio() const { return m_Bio; }
    [[nodiscard]] const std::vector<Role> &getRoles() const { return m_Roles; }
    [[nodiscard]] const std::vector<std::string> &getMessageHistory() const { return m_MessageHistory; }

    void addMessageToHistory(const std::string &message);
};