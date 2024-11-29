//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Connection.h"
#include "ServerConnection.h"
#include "MessageHandler.h"
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

class ConnectionManager {
public:
    ConnectionManager(Server &server, ServerConnection *serverConnection);
    ~ConnectionManager();

private:
    Server &m_Server;

    int m_ServerSocketID;
    std::unordered_map<int, Connection *> m_Connections;
    mutable std::mutex m_ConnectionMutex;

    std::thread m_ConnectionHandlerThread;
    std::thread m_ConnectionTimeoutThread;
    mutable std::mutex m_ConnectionTimeoutMutex;
    mutable std::condition_variable m_ConnectionTimeoutCV;

public:
    size_t size() const;
    bool empty() const;
    std::unordered_map<int, Connection *>::iterator begin();
    std::unordered_map<int, Connection *>::iterator end();

    int getServerSocketID() const;
    std::mutex &getConnectionMutex() const { return m_ConnectionMutex; }

    bool addConnection(Connection *connection);
    bool removeConnection(int socketID);
    Connection *getConnection(int socketID);
    Connection *operator[](int socketID);

    void acceptConnection();
    void handleConnections();
    void checkConnectionTimeouts();

    void pushMessage(Connection &connection, const std::string &message);
};