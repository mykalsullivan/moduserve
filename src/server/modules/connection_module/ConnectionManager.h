//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <mutex>
#include <common/Connection.h>
#include <entt/entt.hpp>

// Forward declaration(s)
class OldConnection;

class ConnectionManager {
    entt::registry m_Connections;
    mutable std::mutex m_Mutex;

public:
    ConnectionManager();
    ~ConnectionManager();

    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;

    bool add(OldConnection &connection);
    bool remove(int socketFD);
    [[nodiscard]] OldConnection *get(int fd);
    [[nodiscard]] OldConnection *operator[](int fd);
};