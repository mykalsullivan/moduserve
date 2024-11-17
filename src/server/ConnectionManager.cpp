//
// Created by msullivan on 11/10/24.
//

#include "ConnectionManager.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "ServerConnection.h"
#include "Server.h"
#include <cstring>

ConnectionManager::ConnectionManager(Server &server, ServerConnection *serverConnection) : m_Server(server)
{
    // Start non-blocking mode
    serverConnection->setNonBlocking();

    // Set server connection
    addConnection(serverConnection);
    m_ServerSocketID = serverConnection->getSocket();

    m_Running = true;
    Logger::instance().logMessage(LogLevel::DEBUG, "Starting connection manager...");

    // Start acceptor thread
    m_ConnectionAcceptorThread = std::thread([this] {
        Logger::instance().logMessage(LogLevel::DEBUG, "(ConnectionManager) Started acceptor thread");
        while (m_Running)
        {
            acceptConnection();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        Logger::instance().logMessage(LogLevel::INFO, "(ConnectionManager) Stopped acceptor thread");
    });

    // Start handler thread
    m_ConnectionHandlerThread = std::thread([this] {
        Logger::instance().logMessage(LogLevel::DEBUG, "(ConnectionManager) Started handler thread");
        int pollRate = 100;
        while (m_Running)
        {
            handleConnections();
            std::this_thread::sleep_for(std::chrono::milliseconds(pollRate));
        }
        Logger::instance().logMessage(LogLevel::INFO, "(ConnectionManager) Stopped handler thread");
    });

    // Start timeout thread
    m_ConnectionTimeoutThread = std::thread([this] {
        Logger::instance().logMessage(LogLevel::DEBUG, "(ConnectionManager) Started timeout thread");
        while (m_Running)
        {
            // Ignore for now
            //checkConnectionTimeouts();
            std::this_thread::sleep_for(std::chrono::seconds(10));  // Check every 10 seconds
        }
        Logger::instance().logMessage(LogLevel::INFO, "(ConnectionManager) Stopped timeout thread");
    });
    Logger::instance().logMessage(LogLevel::DEBUG, "Started connection manager");
}

ConnectionManager::~ConnectionManager() = default;

void ConnectionManager::stop()
{
    Logger::instance().logMessage(LogLevel::DEBUG, "Stopping connection manager...");

    m_Running = false;
    if (m_ConnectionAcceptorThread.joinable())
        m_ConnectionAcceptorThread.join();
    if (m_ConnectionHandlerThread.joinable())
        m_ConnectionHandlerThread.join();
    if (m_ConnectionTimeoutThread.joinable())
        m_ConnectionTimeoutThread.join();

    // Disconnect all connections
    for (auto connection : m_Connections)
    {
        if (connection.first == m_ServerSocketID) continue; // Skip server connection
        removeConnection(connection.first);
    }
    removeConnection(m_ServerSocketID); // Remove the server connection last
    m_Connections.clear();

    Logger::instance().logMessage(LogLevel::DEBUG, "Stopped connection manager");
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
    //Logger::instance().logMessage(LogLevel::DEBUG, "Attempting to add connection...");
    std::lock_guard lock(m_ConnectionMutex);

    int socketID = connection->getSocket();
    //Logger::instance().logMessage(LogLevel::DEBUG, "socketID = " + std::to_string(socketID));

    if (m_Connections.contains(socketID)) {
        Logger::instance().logMessage(LogLevel::ERROR, "Connection already exists");
        return false; // Connection already exists
    }
    m_Connections[socketID] = connection;
    //Logger::instance().logMessage(LogLevel::DEBUG, "Connection added");
    return true;
}

bool ConnectionManager::removeConnection(int socketID)
{
    std::lock_guard lock(m_ConnectionMutex);

    auto it = m_Connections.find(socketID);
    if (it != m_Connections.end()) {
        close(it->first);
        delete it->second;  // Clean up the connection
        m_Connections.erase(it);
        Logger::instance().logMessage(LogLevel::DEBUG, "Connection removed for socket ID: " + std::to_string(socketID));
        return true;
    }
    Logger::instance().logMessage(LogLevel::ERROR, "Connection not found for socket ID: " + std::to_string(socketID));
    return false; // Connection not found
}

Connection *ConnectionManager::getConnection(int socketID)
{
    std::lock_guard lock(m_ConnectionMutex);

    auto it = m_Connections.find(socketID);
    if (it != m_Connections.end()) {
        return it->second;
    }
    return nullptr;
}

Connection *ConnectionManager::operator[](int socketID)
{
    std::lock_guard lock(m_ConnectionMutex);

    auto it = m_Connections.find(socketID);
    if (it != m_Connections.end()) {
        return it->second;
    }
    return nullptr;
}

int ConnectionManager::getServerSocketID() const
{
    std::lock_guard lock(m_ConnectionMutex);
    return m_ServerSocketID;
}


void ConnectionManager::acceptConnection()
{
    if (!m_Running) return;

    //Logger::instance().logMessage(LogLevel::DEBUG, "Attempting to accept connection...");

    pollfd pfd {};
    pfd.fd = m_ServerSocketID;
    pfd.events = POLLIN;

    int result = poll(&pfd, 1, 500);
    if (result == -1) {
        Logger::instance().logMessage(LogLevel::ERROR, "Poll failed on server socket.");
        return;
    }
    if (result == 0) {
        // No incoming connections, just continue
        return;
    }

    // If poll() incidicates that the server socket is ready
    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientFD = -1;
    if (pfd.revents & POLLIN) {
        // Accept the new client connection and add to the manager
        clientFD = accept(m_ServerSocketID, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);

        // Check for errors in accepting connection
        if (clientFD == -1) {
            // If no connection is ready, errno will be EAGAIN or EWOULDBLOCK
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                Logger::instance().logMessage(LogLevel::ERROR, "No data available for connection.");
                return;
            }
            Logger::instance().logMessage(LogLevel::ERROR, "Failed to accept client connection.");
            return;
        }
    }

    // Create new connection
    auto client = new Connection();
    client->setSocket(clientFD);
    client->setAddress(clientAddress);
    client->setNonBlocking();

    if (!addConnection(client)) {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to add connection.");
        delete client;
        return;
    }
    Logger::instance().logMessage(LogLevel::DEBUG, "Client accepted: " + client->getIP() + ":" + std::to_string(client->getPort()));
    m_Server.messageHandler().broadcastMessage(m_Connections[m_ServerSocketID], "Client " + std::to_string(clientFD) + " (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") connected");
}

void ConnectionManager::handleConnections()
{
    //Logger::instance().logMessage(LogLevel::DEBUG, "Handling...");

    // Clear and then populate pollfds for each active connection
    m_PollFDs.clear();
    for (auto pair : m_Connections)
    {
        auto connection = pair.second;

        // Skip the server socket
        if (pair.first == m_ServerSocketID) continue;

        pollfd pfd {};
        pfd.fd = connection->getSocket();       // Set the file descriptor
        pfd.events = POLLIN;                    // Make it poll events
        pfd.revents = 0;                        // Initialize the events to 0

        m_PollFDs.emplace_back(pfd);
    }

    int result = poll(m_PollFDs.data(), m_PollFDs.size(), 100);  // Block indefinitely or set a timeout
    if (result < 0) {
        Logger::instance().logMessage(LogLevel::ERROR, "Poll failed: " + std::string(strerror(errno)));
        return;
    }
    if (result == 0) {
        //Logger::instance().logMessage(LogLevel::DEBUG, "No available data. Skipping...");
        return;
    }

    // Handle events
    for (auto &pollFD : m_PollFDs)
    {
        if (pollFD.revents & POLLIN) {
            auto connection = m_Connections[pollFD.fd];
            if (connection != nullptr) {
                std::string message = connection->receiveMessage();
                if (message.empty()) {
                    Logger::instance().logMessage(LogLevel::INFO, "Client (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ")'s connection reset (disconnected)");
                    m_Server.messageHandler().broadcastMessage(connection, "Client (" + connection->getIP() + ':' + std::to_string(connection->getPort()) + ")'s connection reset (disconnected)");
                    removeConnection(connection->getSocket());
                }
                else {
                    //Logger::instance().logMessage(LogLevel::DEBUG, "Placing message on the queue...");
                    pushMessageToQueue(connection, message);
                }
            }
        }
    }
}

void ConnectionManager::pushMessageToQueue(Connection *connection, const std::string &message)
{
    {
        std::lock_guard lock(m_MessageQueueMutex);
        m_MessageQueue.emplace(connection, message);
    }
    m_MessageQueueCV.notify_one(); // Notify the MessageHandler to process the message
    //Logger::instance().logMessage(LogLevel::DEBUG, "Notified MessageHandler that a message has been received");
}


void ConnectionManager::checkConnectionTimeouts() {

    // Example check: Iterate through connections and perform timeout logic
    for (auto &pair : m_Connections) {
        auto connection = pair.second;

        // Don't time out the server, obviously
        if (connection->getSocket() != m_ServerSocketID) {
            if (connection->isInactive()) {
                Logger::instance().logMessage(LogLevel::INFO, "Client @ " + connection->getIP() + ':' + std::to_string(connection->getPort()) + " timed out; disconnecting...");
                removeConnection(pair.first);  // Timeout connection
            }
        }
    }
}