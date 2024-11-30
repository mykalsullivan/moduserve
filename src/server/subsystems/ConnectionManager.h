//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "Subsystem.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <barrier>

// Forward declaration(s)
class BroadcastManager;
class ServerConnection;
class MessageProcessor;
class Connection;

class ConnectionManager : public Subsystem {
public:
    ConnectionManager(BroadcastManager &broadcastManager,
                        MessageProcessor &messageProcessor,
                        ServerConnection &serverConnection);
    ~ConnectionManager() override;

private:
    BroadcastManager &m_BroadcastManager;
    MessageProcessor &m_MessageProcessor;

    std::unordered_map<int, Connection *> m_Connections;
    int m_ServerFD;

    std::thread m_AcceptorThread;
    std::thread m_EventThread;
    std::barrier<> m_ThreadBarrier;

    mutable std::mutex m_Mutex;
    std::condition_variable m_EventCV;

public:
    int init() override;
    [[nodsciard]] std::string name() override { return "connectionManager"; }

    [[nodiscard]] size_t size() const { return m_Connections.size(); }
    [[nodiscard]] bool empty() const { return m_Connections.empty(); }
    [[nodiscard]] std::unordered_map<int, Connection *>::iterator begin() { return m_Connections.begin(); }
    [[nodiscard]] std::unordered_map<int, Connection *>::iterator end() { return m_Connections.end(); }
    [[nodiscard]] int serverFD() const { return m_ServerFD; }

    bool add(Connection &connection);
    bool remove(int socketFD);
    [[nodiscard]] Connection *get(int fd);
    [[nodiscard]] Connection *operator[](int fd);

private:
    void eventThreadWork();
    void acceptorThreadWork();

    std::vector<int> processConnections();
    void validateConnections();
    void purgeConnections(const std::vector<int> &connectionsToPurge);
    void processMessage(Connection &connection, const std::string &message);
};