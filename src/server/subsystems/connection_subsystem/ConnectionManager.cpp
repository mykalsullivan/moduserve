//
// Created by msullivan on 12/1/24.
//

#include "ConnectionManager.h"
#include "common/Logger.h"
#include "common/Connection.h"
#include "ServerConnection.h"
#include "server/Server.h"
#include "server/Signal.h"

int port = 8000;

ConnectionManager::ConnectionManager()
{
    // 1. Create server connection
    auto serverConnection = new ServerConnection();

    // 2. Create socket
    if (!serverConnection->createSocket())
    {
        logMessage(LogLevel::ERROR, "Failed to create socket");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::DEBUG, "Created server socket: " + std::to_string(serverConnection->getFD()));

    // 3. Create server address
    if (!serverConnection->createAddress(port))
    {
        logMessage(LogLevel::ERROR, "Failed to create address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::DEBUG, "Successfully created server address (listening on all interfaces)");

    // 4. Bind socket to address
    if (!serverConnection->bindAddress())
    {
        logMessage(LogLevel::ERROR, "Failed to bind address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::DEBUG, "Successfully bound to address (" + serverConnection->getIP() + ':' + std::to_string(serverConnection->getPort()) + ')');

    // 5. Listen to incoming connections
    if (!serverConnection->startListening())
    {
        logMessage(LogLevel::ERROR, "Cannot listen to incoming connections");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::INFO, "Listening for new connections on " + serverConnection->getIP() + ':' +
                               std::to_string(serverConnection->getPort()) + ')');

    add(*serverConnection);
    m_ServerFD = serverConnection->getFD();
}

ConnectionManager::~ConnectionManager()
{
    // Disconnect all clients
    std::lock_guard lock(m_Mutex); // Protect access to m_Connections
    for (auto it = m_Connections.begin(); it != m_Connections.end();)
    {
        if (it->first != m_ServerFD)
        {
            delete it->second; // Free memory for the connection
            it = m_Connections.erase(it); // Erase returns the next valid iterator
        }
        else
            ++it;
    }

    // Shut the server connection down
    if (m_Connections.contains(m_ServerFD))
    {
        delete m_Connections[m_ServerFD];
        m_Connections.erase(m_ServerFD);
    }
}

bool ConnectionManager::add(Connection &connection)
{
    std::lock_guard lock(m_Mutex);

    int socketID = connection.getFD();
    if (!m_Connections.contains(socketID))
    {
        m_Connections[socketID] = &connection;
        auto signal = server.signalManager().getSignal<Signal<const Connection &>>("onConnect");
        if (signal) signal->emit(connection);
        return true;
    }
    logMessage(LogLevel::ERROR, "Attempting to add an existing connection");
    return false;
}

bool ConnectionManager::remove(int socketFD)
{
    std::lock_guard lock(m_Mutex);

    auto it = m_Connections.find(socketFD);
    if (it != m_Connections.end())
    {
        auto signal = server.signalManager().getSignal<Signal<const Connection &>>("onDisconnect");
        if (signal) signal->emit(*it->second);

        delete it->second;
        m_Connections.erase(it);
        return true;
    }
    logMessage(LogLevel::ERROR, "Attempting to remove non-existent socket: " + std::to_string(socketFD));
    return false;
}

Connection *ConnectionManager::get(int fd)
{
    auto it = m_Connections.find(fd);
    return (it != m_Connections.end()) ? it->second : nullptr;
}

Connection *ConnectionManager::operator[](int fd)
{
    return get(fd);
}