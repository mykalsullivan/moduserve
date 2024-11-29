//
// Created by msullivan on 11/10/24.
//

#include "ConnectionManager.h"
#include "ServerConnection.h"
#include "Server.h"
#include "MessageHandler.h"
#include "../Logger.h"
#include <cstring>
#include <poll.h>

ConnectionManager::ConnectionManager(Server &server, ServerConnection *serverConnection) : m_Server(server)
{
    LOG(LogLevel::DEBUG, "Starting connection manager...");

    // Set server connection
    addConnection(serverConnection);
    m_ServerSocketID = serverConnection->getSocket();

    // Start event loop thread
    m_EventLoopThread = std::thread(&ConnectionManager::eventLoopWork, this);

    LOG(LogLevel::DEBUG, "Started connection manager");
}

ConnectionManager::~ConnectionManager()
{
    LOG(LogLevel::DEBUG, "Stopping connection manager...");

    // Stop and join the event loop
    m_EventCV.notify_all();
    if (m_EventLoopThread.joinable()) m_EventLoopThread.join();

    // Disconnect all connections
    std::lock_guard lock(m_Mutex);
    for (auto connection : m_Connections)
        if (connection.first != m_ServerSocketID)
            removeConnection(connection.first);

    // Shut the server connection down
    removeConnection(m_ServerSocketID); // Remove the server connection last
    m_Connections.clear();

    LOG(LogLevel::DEBUG, "Stopped connection manager");
}

bool ConnectionManager::addConnection(Connection *connection)
{
    std::lock_guard lock(m_Mutex);

    int socketID = connection->getSocket();
    if (!m_Connections.contains(socketID))
    {
        m_Connections[socketID] = connection;
        m_PollFDs.emplace_back(socketID, POLLIN, 0);

        // Notify the event loop that work is available
        m_EventCV.notify_one();
        return true;
    }
    LOG(LogLevel::ERROR, "Connection already exists");
    return false; // Connection already exists
}

bool ConnectionManager::removeConnection(int socketFD)
{
    std::lock_guard lock(m_Mutex);

    auto it = m_Connections.find(socketFD);
    if (it != m_Connections.end())
    {
        delete it->second;
        m_Connections.erase(it);

        m_EventCV.notify_one();
        LOG(LogLevel::DEBUG, "Connection removed for socket ID: " + std::to_string(socketFD));
        return true;
    }
    LOG(LogLevel::ERROR, "Connection not found for socket ID: " + std::to_string(socketFD));
    return false; // Connection not found
}

Connection *ConnectionManager::getConnection(int socketFD)
{
    std::lock_guard lock(m_Mutex);
    auto it = m_Connections.find(socketFD);
    return (it != m_Connections.end()) ? it->second : nullptr;
}

Connection *ConnectionManager::operator[](int socketFD)
{
    std::lock_guard lock(m_Mutex);
    auto it = m_Connections.find(socketFD);
    return (it != m_Connections.end()) ? it->second : nullptr;
}

void ConnectionManager::eventLoopWork()
{
    LOG(LogLevel::INFO, "Started ConnectionManager event loop");

    while (m_Server.m_Running)
    {
        LOG(LogLevel::DEBUG, "Waiting for event...");

        std::unique_lock lock(m_Mutex);
        m_EventCV.wait(lock, [this] {
            return !m_Server.m_Running || !m_Connections.empty();
        });
        if (!m_Server.m_Running) break;

        std::vector<pollfd> pollFDs;
        for (auto &pair : m_Connections)
        {
            pollfd pfd {};
            pfd.fd = pair.second->getSocket();
            pfd.events = POLLIN;
            pollFDs.emplace_back(pfd);
        }
        lock.unlock();

        int result = poll(pollFDs.data(), pollFDs.size(), 1000); // 1-second timeout
        if (result < 0)
        {
            if (errno == EINTR) continue; // Retry on signal interrupation
            LOG(LogLevel::ERROR, "Poll error: " + std::string(strerror(errno)));
            break;
        }

        // Handle events
        lock.lock();
        for (auto &pfd : pollFDs)
        {
            if (pfd.revents & POLLIN)
            {
                auto connection = m_Connections[pfd.fd];
                if (connection)
                {
                    std::string message = connection->receiveMessage();
                    if (!message.empty())
                        processMessage(*connection, message);
                    else
                    {
                        LOG(LogLevel::INFO, "Client disconnected: " + connection->getIP());
                        removeConnection(pfd.fd);
                    }
                }
            }
        }
        lock.unlock();
        checkConnectionTimeouts(); // Check for timeouts
    }
    LOG(LogLevel::INFO, "Stopped ConnectionManager event loop");
}

void ConnectionManager::acceptorThreadWork()
{
    while (m_Server.m_Running)
    {

    }
}


void ConnectionManager::acceptConnection()
{
    LOG(LogLevel::DEBUG, "Attempting to accept connection...");

    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientFD = accept(m_ServerSocketID, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
    if (clientFD >= 0)
    {
        auto client = new Connection();
        client->setSocket(clientFD);
        client->setAddress(clientAddress);

        if (addConnection(client))
        {
            LOG(LogLevel::DEBUG, "Client accepted: " + client->getIP() + ":" + std::to_string(client->getPort()));
            m_Server.m_MessageHandler.broadcastMessage(*m_Connections[m_ServerSocketID], "Client " + std::to_string(clientFD) + " (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") connected");
            m_EventCV.notify_one();
        }
        else
        {
            LOG(LogLevel::ERROR, "Failed to add connection.");
            delete client;
        }
    }
    else
        LOG(LogLevel::ERROR, "Failed to accept client connection.");
}

void ConnectionManager::processMessage(Connection &connection, const std::string &message)
{
    m_Server.m_MessageHandler.handleMessage(connection, message);
}


void ConnectionManager::checkConnectionTimeouts()
{
    std::lock_guard lock(m_Mutex);

    for (auto it = m_Connections.begin(); it != m_Connections.end();)
    {
        // Don't time out the server, obviously
        if (it->first != m_ServerSocketID && it->second->isInactive(30))
        {
            LOG(LogLevel::INFO, "Client @ " + it->second->getIP() + ':' + std::to_string(it->second->getPort()) + " timed out; disconnecting...");
            removeConnection(it->first);  // Timeout connection
            it = m_Connections.erase(it);
        }
        else
            ++it;
    }
}