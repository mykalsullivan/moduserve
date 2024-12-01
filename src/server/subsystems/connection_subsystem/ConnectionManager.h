//
// Created by msullivan on 12/1/24.
//

#pragma once
#include <unordered_map>
#include <mutex>

// Forward declaration(s)
class Connection;
class ServerConnection;

class ConnectionManager {
    std::unordered_map<int, Connection *> m_Connections;
    int m_ServerFD;

    mutable std::mutex m_Mutex;

public:
    ConnectionManager();
    ~ConnectionManager();

    [[nodiscard]] size_t size() const { return m_Connections.size(); }
    [[nodiscard]] bool empty() const { return m_Connections.empty(); }
    [[nodiscard]] auto begin() { return m_Connections.begin(); }
    [[nodiscard]] auto end() { return m_Connections.end(); }
    [[nodiscard]] int serverFD() const { return m_ServerFD; }

    bool add(Connection &connection);
    bool remove(int socketFD);
    [[nodiscard]] Connection *get(int fd);
    [[nodiscard]] Connection *operator[](int fd);
};