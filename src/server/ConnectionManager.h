//
// Created by msullivan on 11/10/24.
//

#pragma once
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

class ConnectionManager {
public:
    ConnectionManager(BroadcastManager &broadcastManager,
                        MessageProcessor &messageProcessor,
                        ServerConnection &serverConnection,
                        std::barrier<> &serviceBarrier);
    ~ConnectionManager();

private:
    BroadcastManager &m_BroadcastManager;
    MessageProcessor &m_MessageProcessor;

    std::unordered_map<int, Connection *> m_Connections;
    int m_ServerFD;

    mutable std::mutex m_Mutex;
    std::thread m_EventThread;
    std::condition_variable m_EventCV;

    std::thread m_AcceptorThread;

public:
    size_t size() const { return m_Connections.size(); }
    bool empty() const { return m_Connections.empty(); }
    std::unordered_map<int, Connection *>::iterator begin() { return m_Connections.begin(); }
    std::unordered_map<int, Connection *>::iterator end() { return m_Connections.end(); }
    int serverFD() const { return m_ServerFD; }

    bool addConnection(Connection &connection);
    bool removeConnection(int socketFD);
    Connection *getConnection(int fd);
    Connection *operator[](int fd);

private:
    void eventThreadWork();
    void acceptorThreadWork();

    std::vector<int> processConnections();
    void validateConnections();
    void purgeConnections(const std::vector<int> &connectionsToPurge);
    void processMessage(Connection &connection, const std::string &message);
};