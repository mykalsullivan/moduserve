//
// Created by msullivan on 11/10/24.
//

#include "ConnectionManager.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "ServerConnection.h"
#include "Server.h"
#include <cstring>
#include <poll.h>

ConnectionManager::ConnectionManager(Server &server, ServerConnection *serverConnection) : m_Server(server)
{
    // Start non-blocking mode
    serverConnection->setNonBlocking();

    // Set server connection
    addConnection(serverConnection);
    m_ServerSocketID = serverConnection->getSocket();

    LOG(LogLevel::DEBUG, "Starting connection manager...");

    // Start handler thread
    m_ConnectionHandlerThread = std::thread([this] {
        LOG(LogLevel::DEBUG, "(ConnectionManager) Started handler thread");
        while (m_Server.isRunning())
        {
            acceptConnection();
            handleConnections();
        }
        LOG(LogLevel::INFO, "(ConnectionManager) Stopped handler thread");
    });

    // Start timeout thread
    m_ConnectionTimeoutThread = std::thread([this] {
        LOG(LogLevel::DEBUG, "(ConnectionManager) Started timeout thread");

        std::unique_lock lock(m_ConnectionTimeoutMutex);
        while (m_Server.isRunning())
        {
            m_ConnectionTimeoutCV.wait_for(lock, std::chrono::seconds(10), [this] {
                return !m_Server.isRunning();
            });
            if (m_Server.isRunning())
                checkConnectionTimeouts();
        }
        LOG(LogLevel::INFO, "(ConnectionManager) Stopped timeout thread");
    });

    LOG(LogLevel::DEBUG, "Started connection manager");
}

ConnectionManager::~ConnectionManager()
{
    LOG(LogLevel::DEBUG, "Stopping connection manager...");

    // Disconnect all connections
    for (auto connection : m_Connections)
        if (connection.first != m_ServerSocketID)
        {
            removeConnection(connection.first);
            m_Connections.erase(connection.first);
        }

    // Shut the server connection down
    shutdown(m_ServerSocketID, SHUT_RDWR);
    removeConnection(m_ServerSocketID); // Remove the server connection last
    m_Connections.clear();

    m_ConnectionTimeoutCV.notify_one(); // Wake thread to finish execution

    if (m_ConnectionHandlerThread.joinable())
        m_ConnectionHandlerThread.join();
    if (m_ConnectionTimeoutThread.joinable())
        m_ConnectionTimeoutThread.join();

    LOG(LogLevel::DEBUG, "Stopped connection manager");
}

size_t ConnectionManager::size() const
{
    std::lock_guard lock(m_ConnectionMutex);
    return m_Connections.size();
}

bool ConnectionManager::empty() const
{
    std::lock_guard lock(m_ConnectionMutex);
    return m_Connections.empty();
}


std::unordered_map<int, Connection *>::iterator ConnectionManager::begin()
{
    std::lock_guard lock(m_ConnectionMutex);
    return m_Connections.begin();
}

std::unordered_map<int, Connection *>::iterator ConnectionManager::end()
{
    std::lock_guard lock(m_ConnectionMutex);
    return m_Connections.end();
}

bool ConnectionManager::addConnection(Connection *connection)
{
    //LOG(LogLevel::DEBUG, "Attempting to add connection...");
    std::lock_guard lock(m_ConnectionMutex);

    int socketID = connection->getSocket();
    //LOG(LogLevel::DEBUG, "socketID = " + std::to_string(socketID));

    if (m_Connections.contains(socketID))
    {
        LOG(LogLevel::ERROR, "Connection already exists");
        return false; // Connection already exists
    }
    m_Connections[socketID] = connection;
    m_ConnectionTimeoutCV.notify_one(); // Notify the timeout thread about the new connection
    //LOG(LogLevel::DEBUG, "Connection added");
    return true;
}

bool ConnectionManager::removeConnection(int socketID)
{
    std::lock_guard lock(m_ConnectionMutex);

    auto it = m_Connections.find(socketID);
    if (it != m_Connections.end())
    {
        close(it->first);
        delete it->second;  // Clean up the connection
        m_Connections.erase(it);
        m_ConnectionTimeoutCV.notify_one();
        LOG(LogLevel::DEBUG, "Connection removed for socket ID: " + std::to_string(socketID));
        return true;
    }
    LOG(LogLevel::ERROR, "Connection not found for socket ID: " + std::to_string(socketID));
    return false; // Connection not found
}

Connection *ConnectionManager::getConnection(int socketID)
{
    std::lock_guard lock(m_ConnectionMutex);

    auto it = m_Connections.find(socketID);
    if (it != m_Connections.end())
        return it->second;
    return nullptr;
}

Connection *ConnectionManager::operator[](int socketID)
{
    std::lock_guard lock(m_ConnectionMutex);

    auto it = m_Connections.find(socketID);
    if (it != m_Connections.end())
        return it->second;
    return nullptr;
}

int ConnectionManager::getServerSocketID() const
{
    std::lock_guard lock(m_ConnectionMutex);
    return m_ServerSocketID;
}


void ConnectionManager::acceptConnection()
{
    //LOG(LogLevel::DEBUG, "Attempting to accept connection...");

    pollfd pfd {};
    pfd.fd = m_ServerSocketID;
    pfd.events = POLLIN;

    static int timeoutRate = 500;
    int result = poll(&pfd, 1, timeoutRate);
    if (result == -1)
    {
        LOG(LogLevel::ERROR, "Poll failed on server socket.");
        return;
    }

    static int idleCounter = 0;
    if (result == 0)
    {
        idleCounter++;
        if (idleCounter > 5)
        {
            timeoutRate = std::min(timeoutRate + 100, 2000); // Gradually increase timeout
            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutRate));
        }
        return;
    }
    idleCounter = 0;

    // If poll() incidicates that the server socket is ready
    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientFD = -1;
    if (pfd.revents & POLLIN)
    {
        // Accept the new client connection and add to the manager
        clientFD = accept(m_ServerSocketID, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);

        // Check for errors in accepting connection
        if (clientFD == -1)
        {
            // If no connection is ready, errno will be EAGAIN or EWOULDBLOCK
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                LOG(LogLevel::ERROR, "No data available for connection.");
                return;
            }
            LOG(LogLevel::ERROR, "Failed to accept client connection.");
            return;
        }
    }

    // Create new connection
    auto client = new Connection();
    client->setSocket(clientFD);
    client->setAddress(clientAddress);
    client->setNonBlocking();

    if (!addConnection(client))
    {
        LOG(LogLevel::ERROR, "Failed to add connection.");
        delete client;
        return;
    }
    LOG(LogLevel::DEBUG, "Client accepted: " + client->getIP() + ":" + std::to_string(client->getPort()));
    m_Server.m_MessageHandler.broadcastMessage(*m_Connections[m_ServerSocketID], "Client " + std::to_string(clientFD) + " (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") connected");
}

void ConnectionManager::handleConnections()
{
    //LOG(LogLevel::DEBUG, "Handling...");

    // Clear and then populate pollfds for each active connection
    std::vector<pollfd> pollFDs;
    pollFDs.clear();
    for (auto pair : m_Connections)
    {
        auto connection = pair.second;

        // Skip the server socket
        if (pair.first == m_ServerSocketID) continue;

        pollfd pfd {};
        pfd.fd = connection->getSocket();       // Set the file descriptor
        pfd.events = POLLIN;                    // Make it poll events
        pfd.revents = 0;                        // Initialize the events to 0

        pollFDs.emplace_back(pfd);
    }

    static int timeoutRate = 500; // Timeout rate in milliseconds
    static int idleCount = 0; // Idle connection counter
    int result = poll(pollFDs.data(), pollFDs.size(), timeoutRate);  // Block indefinitely or set a timeout
    if (result < 0)
    {
        LOG(LogLevel::ERROR, "Poll failed: " + std::string(strerror(errno)));
        return;
    }

    // No available data, just continue
    if (result == 0)
    {
        idleCount++;
        if (idleCount > 5)
        {
            timeoutRate = std::min(timeoutRate + 100, 2000); // Gradually increase timeout
            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutRate));
        }
        return;
    }

    timeoutRate = 500;
    idleCount = 500; // Reset to default when activity occurs

    // Handle events
    for (auto &pollFD : pollFDs)
        if (pollFD.revents & POLLIN)
        {
            auto connection = m_Connections[pollFD.fd];
            if (connection != nullptr)
            {
                std::string message = connection->receiveMessage();
                if (message.empty())
                {
                    LOG(LogLevel::INFO, "Client (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ")'s connection reset (disconnected)");
                    m_Server.m_MessageHandler.broadcastMessage(*connection, "Client (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ")'s connection reset (disconnected)");
                    removeConnection(connection->getSocket());
                }
                else
                    pushMessage(*connection, message);
            }
        }
}

void ConnectionManager::pushMessage(Connection &connection, const std::string &message)
{
    m_Server.m_MessageHandler.handleMessage(connection, message);
    //LOG(LogLevel::DEBUG, "Notified MessageHandler that a message has been received");
}


void ConnectionManager::checkConnectionTimeouts()
{
    std::lock_guard lock(m_ConnectionMutex);

    auto now = std::chrono::steady_clock::now();
    for (auto it = m_Connections.begin(); it != m_Connections.end();)
    {
        auto connection = it->second;

        // Don't time out the server, obviously
        if (connection->getSocket() != m_ServerSocketID && connection->isInactive(30))
        {
            LOG(LogLevel::INFO, "Client @ " + connection->getIP() + ':' + std::to_string(connection->getPort()) + " timed out; disconnecting...");
            removeConnection(it->first);  // Timeout connection
            it = m_Connections.erase(it);
        }
        else
            ++it;
    }
}