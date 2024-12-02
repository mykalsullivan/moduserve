//
// Created by msullivan on 11/11/24.
//

#pragma once
#include <string>
#include <vector>

class Role {
public:
    Role(const std::string &name, const std::vector<std::string> &permissions)
        : m_Name(name), m_Permissions(permissions) {}

    [[nodiscard]] const std::string &getName() const { return m_Name; }
    [[nodiscard]] const std::vector<std::string> &getPermissions() const { return m_Permissions; }

private:
    std::string m_Name; // Name of the role (e.g., "Admin", "User ")
    std::vector<std::string> m_Permissions; // List of permissions associated with the role
};