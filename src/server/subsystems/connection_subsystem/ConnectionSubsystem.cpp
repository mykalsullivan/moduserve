//
// Created by msullivan on 11/10/24.
//

#include "ConnectionSubsystem.h"
#include "server/subsystems/broadcast_subsystem/BroadcastSubsystem.h"
#include "server/subsystems/message_subsystem/MessageSubsystem.h"
#include "server/ServerConnection.h"
#include "server/Server.h"
#include "common/Logger.h"
#include <barrier>

ConnectionSubsystem::ConnectionSubsystem(ServerConnection &serverConnection) : m_ThreadBarrier(2)
{
    // Set server connection
    add(serverConnection);
    m_ServerFD = serverConnection.getFD();
}

ConnectionSubsystem::~ConnectionSubsystem()
{
    // Stop and join the event loop
    m_EventCV.notify_all();
    if (m_EventThread.joinable()) m_EventThread.join();

    // Join the acceptor loop
    if (m_AcceptorThread.joinable()) m_AcceptorThread.join();

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
        delete m_Connections[m_ServerFD]; // Free memory for the server socket
        m_Connections.erase(m_ServerFD);
    }

    LOG(LogLevel::INFO, "(ConnectionSubsystem) Started");
}

int ConnectionSubsystem::init()
{
    // Start acceptor thread
    m_AcceptorThread = std::thread(&ConnectionSubsystem::acceptorThreadWork, this);

    // Start event loop thread
    m_EventThread = std::thread(&ConnectionSubsystem::eventThreadWork, this);

    LOG(LogLevel::INFO, "(ConnectionSubsystem) Stopped");
    return 0;
}

bool ConnectionSubsystem::add(Connection &connection)
{
    std::lock_guard lock(m_Mutex);

    int socketID = connection.getFD();
    if (!m_Connections.contains(socketID))
    {
        m_Connections[socketID] = &connection;

        // Notify the event loop that work is available
        m_EventCV.notify_one();
        return true;
    }
    LOG(LogLevel::ERROR, "Attempting to add an existing connection");
    return false; // Connection already exists
}

bool ConnectionSubsystem::remove(int socketFD)
{
    std::lock_guard lock(m_Mutex);

    auto it = m_Connections.find(socketFD);
    if (it != m_Connections.end())
    {
        delete it->second;
        m_Connections.erase(it);

        m_EventCV.notify_one();
        LOG(LogLevel::DEBUG, "Removed socket: " + std::to_string(socketFD));
        return true;
    }
    LOG(LogLevel::ERROR, "Attempting to remove non-existent socket: " + std::to_string(socketFD));
    return false; // Connection not found
}

Connection *ConnectionSubsystem::get(int fd)
{
    std::lock_guard lock(m_Mutex);
    auto it = m_Connections.find(fd);
    return (it != m_Connections.end()) ? it->second : nullptr;
}

Connection *ConnectionSubsystem::operator[](int fd)
{
    return get(fd);
}

void ConnectionSubsystem::eventThreadWork()
{
    // Wait for all threads to be created
    m_ThreadBarrier.arrive_and_wait();

    LOG(LogLevel::DEBUG, "(ConnectionSubsystem) Started event thread")
    while (Server::instance().isRunning())
    {
        std::unique_lock lock(m_Mutex);
        m_EventCV.wait(lock, [this] {
            return !Server::instance().isRunning() || m_Connections.size() > 1;
        });
        if (!Server::instance().isRunning()) break;
        lock.unlock();

        std::vector<int> connectionsToPurge = processConnections();
        purgeConnections(connectionsToPurge);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG(LogLevel::DEBUG, "(ConnectionSubsystem) Stopped event thread")
}

void ConnectionSubsystem::acceptorThreadWork()
{
    // Wait for all threads to be created
    m_ThreadBarrier.arrive_and_wait();

    LOG(LogLevel::DEBUG, "(ConnectionSubsystem) Started acceptor thread")
    while (Server::instance().isRunning())
    {
        if (auto client = reinterpret_cast<ServerConnection *>(m_Connections[m_ServerFD])->acceptClient())
        {
            if (add(*client))
            {
                LOG(LogLevel::INFO, "Client @ " + client->getIP() + ':' + std::to_string(client->getPort()) + " connected");
                //m_BroadcastManager.broadcastMessage(*m_Connections[m_ServerFD],
                //    "Client @ " + std::to_string(client->getFD()) + " (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") connected");
            }
            else
            {
                LOG(LogLevel::ERROR, "Failed to add client connection");
                delete client;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG(LogLevel::DEBUG, "(ConnectionSubsystem) Stopped acceptor thread")
}

void ConnectionSubsystem::processMessage(Connection &connection, const std::string &message)
{
    //m_MessageProcessor.handleMessage(connection, message);
}

void ConnectionSubsystem::validateConnections()
{
    std::vector<int> connectionsToPurge;
    {
        std::lock_guard lock(m_Mutex);

        for (auto &it : m_Connections)
        {
            int fd = it.first;
            if (fd == m_ServerFD) // Skip server
                continue;

            auto connection = it.second;
            if (!connection || !connection->isValid() || connection->isInactive(30)) // Remove invalid connections
                connectionsToPurge.emplace_back(fd);
        }
    }
    purgeConnections(connectionsToPurge);
}

std::vector<int> ConnectionSubsystem::processConnections()
{
    std::lock_guard lock(m_Mutex);

    std::vector<int> connectionsToPurge;
    for (auto it = m_Connections.begin(); it != m_Connections.end();)
    {
        int fd = it->first;
        if (fd == m_ServerFD) // Skip server
        {
            ++it;
            continue;
        }

        auto connection = it->second;
        if (connection->hasPendingData())
        {
            std::string message = connection->receiveData();
            if (!message.empty())
                processMessage(*connection, message);
            else
            {
                LOG(LogLevel::INFO, "Client (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ") disconnected");
                connectionsToPurge.emplace_back(fd);
            }
        }
        ++it;
    }
    return connectionsToPurge;
}

void ConnectionSubsystem::purgeConnections(const std::vector<int> &connectionsToPurge)
{
    for (int fd : connectionsToPurge) remove(fd);
}
