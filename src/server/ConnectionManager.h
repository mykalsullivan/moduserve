//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Connection.h"
#include "ServerConnection.h"
#include "MessageHandler.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <poll.h>

class ConnectionManager {
public:
    ConnectionManager(Server &server, ServerConnection *serverConnection);
    ~ConnectionManager();

private:
    Server &m_Server;

    int m_ServerSocketID;
    std::unordered_map<int, Connection *> m_Connections;
    std::queue<Connection *> m_NewConnections;
    std::vector<pollfd> m_PollFDs;

    mutable std::mutex m_Mutex;
    std::thread m_EventLoopThread;
    std::condition_variable m_EventCV;

    mutable std::mutex m_QueueMutex;
    std::thread m_AcceptorThread;

public:
    size_t size() const { return m_Connections.size(); }
    bool empty() const { return m_Connections.empty(); }
    std::unordered_map<int, Connection *>::iterator begin() { return m_Connections.begin(); }
    std::unordered_map<int, Connection *>::iterator end() { return m_Connections.end(); }
    int getServerSocketID() const { return m_ServerSocketID; }

    bool addConnection(Connection *connection);
    bool removeConnection(int socketFD);
    Connection *getConnection(int socketFD);
    Connection *operator[](int socketFD);

private:
    void eventLoopWork();
    void acceptorThreadWork();

    void acceptConnection();
    void checkConnectionTimeouts();

    void processMessage(Connection &connection, const std::string &message);
};