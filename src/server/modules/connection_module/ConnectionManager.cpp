//
// Created by msullivan on 12/1/24.
//

#include "ConnectionManager.h"
#include "common/Logger.h"
#include "common/Connection.h"
#include "server/Server.h"
#include "server/Signal.h"

ConnectionManager::ConnectionManager()
{}

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

size_t ConnectionManager::size() const
{
    return m_Connections.view<SocketComponent>().size();
}

bool ConnectionManager::empty() const
{
    return m_Connections.view<SocketComponent>().empty();
}


bool ConnectionManager::add(OldConnection &connection)
{
    std::lock_guard lock(m_Mutex);

    int socketID = connection.fd();
    if (!m_Connections.contains(socketID))
    {
        m_Connections[socketID] = &connection;
        auto signal = server.signalManager().get<Signal<const OldConnection &>>("onConnect");
        if (signal) signal->emit(connection);
        return true;
    }
    logMessage(LogLevel::Error, "Attempting to add an existing connection");
    return false;
}

bool ConnectionManager::remove(int socketFD)
{
    std::lock_guard lock(m_Mutex);

    auto it = m_Connections.find(socketFD);
    if (it != m_Connections.end())
    {
        auto signal = server.signalManager().get<Signal<const OldConnection &>>("onDisconnect");
        if (signal) signal->emit(*it->second);

        delete it->second;
        m_Connections.erase(it);
        return true;
    }
    logMessage(LogLevel::Error, "Attempting to remove non-existent socket: " + std::to_string(socketFD));
    return false;
}

OldConnection *ConnectionManager::get(int fd)
{
    auto it = m_Connections.find(fd);
    return (it != m_Connections.end()) ? it->second : nullptr;
}

OldConnection *ConnectionManager::operator[](int fd)
{
    return get(fd);
}