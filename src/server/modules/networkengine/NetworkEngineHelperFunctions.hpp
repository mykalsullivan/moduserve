//
// Created by msullivan on 12/6/24.
//

#pragma once
#include "NetworkEngine.h"
#include <cstring>
#include <condition_variable>
#include <barrier>
#include <fcntl.h>
#include <netinet/in.h>
#include <entt/entt.hpp>

#include "server/modules/logger/Logger.h"

// Forward declaration(s)
struct ServerConnection {};
struct ServerInfo {
    int maxConnections = -1;
};

struct ClientConnection {};
struct ClientInfo {
    std::chrono::steady_clock::time_point lastActivityTime = std::chrono::steady_clock::now();
};

struct SocketInfo {
#ifndef _WIN32
    int fd = -1;
#else
    SOCKET fd = -1;
#endif
    sockaddr_in address {};
};

struct Metrics {
    size_t bytesSent = 0;
    size_t bytesReceived = 0;
};

void validateConnections();
void processConnections();
void processConnectionsInternal(const std::function<bool(Connection)> &predicate);

Connection createConnection(bool, int);
entt::entity createConnectionEntity(int, sockaddr_in, bool);
int createAndConfigureSocket(bool isServer, int port);
bool createServerAddress(sockaddr_in &address, int port);
bool bindAddress(int serverFD, sockaddr_in serverAddress);
bool startListening(int serverFD);
bool acceptClient();
entt::entity fdToEntity(Connection connection);
Connection entityToFD(entt::entity entity);
bool isValid(Connection connection);

// Global variables
extern entt::registry g_ConnectionRegistry;
extern Connection g_ServerConnection;
extern std::condition_variable g_NetworkEngineCV;

inline void processConnectionsInternal(const std::function<bool(Connection)> &predicate)
{
    //Logger::log(LogLevel::DEBUG, "Checking if connections need to be purged...");

    std::vector<Connection> connectionsToPurge;

    auto clients = g_ConnectionRegistry.view<ClientConnection>();
    for (auto client: clients)
        if (predicate(entityToFD(client)))
            connectionsToPurge.emplace_back(entityToFD(client));

    for (auto client: connectionsToPurge)
        NetworkEngine::disconnect(client);

    // Notify the event thread
    g_NetworkEngineCV.notify_one();
}

inline void validateConnections()
{
    //Logger::log(LogLevel::DEBUG, "Validating connections...");
    processConnectionsInternal([](Connection connection)
    {
        // Purge invalid or inactive connections
        bool isValid = NetworkEngine::isValidConnection(connection);
        bool isActive = NetworkEngine::isActiveConnection(connection, 30);
        return !(isValid && isActive);
    });
}

inline void processConnections()
{
    auto clientView = g_ConnectionRegistry.view<ClientConnection>();

    // Check and process connections with data
    for (auto clientEntity : clientView)
    {
        // This way of checking if there is pending data is terrible
        Connection client = entityToFD(clientEntity);
        processConnectionsInternal([&](Connection)
        {
            std::string message = NetworkEngine::receiveData(client);

            if (!NetworkEngine::hasPendingData(client) && message.empty())
                return false; // Mark for purging

            // Process message if received
            if (!message.empty())
                return false;
            return false; // Purge if no message is received and there is no pending data
        });
    }
}

[[nodiscard]] inline Connection createConnection(bool isServer, int port = 0)
{
    int fd = createAndConfigureSocket(isServer, port);
    if (fd == -1)
    {
        Logger::log(LogLevel::Error, "Failed to create socket for connection");
        return -1;
    }

    sockaddr_in clientAddress {}; // Can be empty for now
    auto connection = createConnectionEntity(fd, clientAddress, isServer);

    // For servers, just return the FD; no entity is needed
    if (isServer && g_ServerConnection != -1)
    {
        g_ServerConnection = entityToFD(connection);
        Logger::log(LogLevel::Info, "Server connection entity created");
    }
    return entityToFD(connection);
}

[[nodiscard]] inline entt::entity createConnectionEntity(int fd, sockaddr_in clientAddress, bool isServer = false)
{
    // Create entity
    auto connection = g_ConnectionRegistry.create();

    if (isServer && g_ServerConnection != -1)
    {
        g_ConnectionRegistry.emplace<ServerConnection>(connection);
        g_ConnectionRegistry.emplace<ServerInfo>(connection);
    }
    else
    {
        g_ConnectionRegistry.emplace<ClientConnection>(connection);
        g_ConnectionRegistry.emplace<ClientInfo>(connection);
    }
    g_ConnectionRegistry.emplace<SocketInfo>(connection, fd, clientAddress);
    g_ConnectionRegistry.emplace<Metrics>(connection);

    return connection;
}

// Accepts a client and returns a Connection ID if successful
inline bool acceptClient()
{
    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);

    auto serverFD = NetworkEngine::getFD(g_ServerConnection);
    int clientFD = accept(serverFD, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
    if (clientFD == -1) return false;

    // Set client FD to non-blocking
#ifndef _WIN32
    int flags = fcntl(clientFD, F_GETFL, 0);
    if (flags == -1 || fcntl(clientFD, F_SETFL, flags | O_NONBLOCK) == -1)
    {
#else
    u_long mode = 1; // 1 = non-blocking
    if (ioctlsocket(clientFD, FIONBIO, &mode) != 0)
    {
#endif
        Logger::log(LogLevel::Error, "Failed to set client FD to non-blocking mode");
        close(clientFD);
        return false;
    }

    // Create connection entity
    auto client = createConnectionEntity(clientFD, clientAddress);

    NetworkEngine::clientAccepted(entityToFD(client));
    return true;
}

[[nodiscard]] inline int createAndConfigureSocket(bool isServer, int port = 0)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
#ifndef _WIN32
        Logger::log(LogLevel::Error, "Socket creation failed: " + std::string(strerror(errno)));
#else
        Logger::log(LogLevel::Error, "Socket creation failed: " + std::to_string(WSAGetLastError()));
#endif
        return -1;
    }

    // Set to non-blocking mode
#ifndef _WIN32
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
#else
    u_long mode = 1; // 1 = non-blocking
    if (ioctlsocket(fd, FIONBIO, &mode) != 0)
    {
#endif
        Logger::log(LogLevel::Error, "Failed to set socket to non-blocking mode");
        close(fd);
        return -1;
    }

    // Enable keepalive
    int enableKeepalive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepalive, sizeof(enableKeepalive));

    if (isServer)
    {
        // Configure server-specific settings
        sockaddr_in serverAddress {};
        if (!createServerAddress(serverAddress, port))
        {
            Logger::log(LogLevel::Error, "Failed to create server address");
            close(fd);
            return -1;
        }

        if (!bindAddress(fd, serverAddress))
        {
#ifndef _WIN32
            Logger::log(LogLevel::Error, "Bind failed: " + std::string(strerror(errno)));
#else
            Logger::log(LogLevel::Error, "Bind failed: " + std::to_string(WSAGetLastError()));
#endif
            close(fd);
            return -1;
        }

        if (!startListening(fd))
        {
#ifndef _WIN32
            Logger::log(LogLevel::Error, "Listen failed: " + std::string(strerror(errno)));
#else
            Logger::log(LogLevel::Error, "Listen failed: " + std::to_string(WSAGetLastError()));
#endif
            close(fd);
            return -1;
        }
        Logger::log(LogLevel::Info, "Server socket configured to listen on port " + std::to_string(port));
    }
    return fd;
}

// Creates an object
inline int createSocket()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

// Creates a server sockaddr_in object
inline bool createServerAddress(sockaddr_in &address, int port)
{
    sockaddr_in serverAddress {};
    serverAddress.sin_family = AF_INET;             // Use IPv4
    serverAddress.sin_port = htons(port);           // Bind to port; include bounds checking in the future
    serverAddress.sin_addr.s_addr = INADDR_ANY;     // Bind to all interfaces
    address = serverAddress;
    return true;
}

// Binds the server file descriptor to the server address
inline bool bindAddress(int serverFD, sockaddr_in serverAddress)
{
    if (bind(serverFD, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) >= 0) [[likely]] return true;
    return false;
}

// Makes the server file descripter listen for connections
inline bool startListening(int serverFD)
{
    if (listen(serverFD, 5) >= 0) [[likely]] return true;
    return false;
}

// Returns the file descriptor of a specified entity
inline Connection entityToFD(entt::entity entity)
{
    return g_ConnectionRegistry.get<SocketInfo>(entity).fd;
}

inline entt::entity fdToEntity(Connection connection)
{
    for (const entt::entity ent : g_ConnectionRegistry.view<SocketInfo>())
        if (g_ConnectionRegistry.get<SocketInfo>(ent).fd == connection)
            return ent;
    return entt::null;
}

// Checks if a connection is valid
inline bool isValid(Connection connection)
{
    entt::entity connectionEntity = fdToEntity(connection);
    return g_ConnectionRegistry.valid(connectionEntity) && g_ConnectionRegistry.get<SocketInfo>(connectionEntity).fd != -1;
}