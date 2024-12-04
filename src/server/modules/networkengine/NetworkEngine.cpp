//
// Created by msullivan on 11/10/24.
//

#include "NetworkEngine.h"
#include "server/Server.h"
#include "common/Logger.h"
#include "common/Message.h"
#include "Connection.h"
#include <future>
#include <barrier>
#include <fcntl.h>
#include <entt/entt.hpp>

#include "server/modules/messageprocessor/MessageProcessor.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

entt::registry g_Registry;
std::mutex g_Mutex;
Connection g_ServerConnection = -1;
int port = 8000;

std::mutex m_ThreadMutex;
std::condition_variable m_ThreadCV;
std::barrier m_ThreadBarrier(2);


// Forward declaration(s)
std::future<void> acceptorCoroutineFunc();
std::future<void> eventCoroutineFunc();
void validateConnections();
void processConnections();
void processConnectionsInternal(const std::function<bool(Connection)> &predicate);
void onDestroyFunction(const SocketInfo &socket);
int createSocket();
bool createServerAddress(sockaddr_in &address, int port);
bool bindAddress(int serverFD, sockaddr_in serverAddress);
bool startListening(int serverFD);
Connection acceptClient();
bool isValid(Connection connection);
Connection createConnection();

NetworkEngine::NetworkEngine()
{
    // 1. Create server connection
    auto serverConnection = g_Registry.create();

    // 2. Create file descriptor
    SocketInfo socket {};
    socket.fd = -1;
    if ((socket.fd = createSocket()) == -1)
    {
        logMessage(LogLevel::Error, "Failed to create socket");
        g_Registry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Debug, "Created server socket: " + std::to_string(socket.fd));

    // 3. Create server address
    int port = 8000;
    if (!createServerAddress(socket.address, port))
    {
        logMessage(LogLevel::Error, "Failed to create address");
        g_Registry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Debug, "Successfully created server address (listening on all interfaces)");

    // 4. Bind socket to address
    if (!bindAddress(socket.fd, socket.address))
    {
        logMessage(LogLevel::Error, "Failed to bind address");
        g_Registry.destroy(serverConnection);
        g_Registry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    std::string ip = getIP(static_cast<Connection>(serverConnection));
    logMessage(LogLevel::Debug, "Successfully bound to address (" + ip + ':' + std::to_string(port + ')'));

    // 5. Listen to incoming connections
    if (!startListening(socket.fd))
    {
        logMessage(LogLevel::Error, "Cannot listen to incoming connections");
        g_Registry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Info, "Listening for new connections on " + ip + ':' + std::to_string(port) + ')');


    // 6. Bind server connection components
    g_Registry.emplace<ServerConnection>(serverConnection);
    g_Registry.emplace<ServerInfo>(serverConnection);
    g_Registry.emplace<Metrics>(serverConnection);

    g_ServerConnection = static_cast<Connection>(serverConnection);
}

NetworkEngine::~NetworkEngine()
{
    // Stop and join the event loop
    m_ThreadCV.notify_all();

    acceptorCoroutineFunc();        // Wait for acceptor coroutine to finish
    eventCoroutineFunc();           // Wait for event coroutine to finish

#ifdef WIN32
    WSACleanup();
#endif

    // Cleanup connections
    auto clients = g_Registry.view<ClientConnection>();

    std::lock_guard lock(g_Mutex); // Protect access to m_Connections
    for (auto client : clients)
        disconnect(static_cast<Connection>(client));

    g_Registry.clear();

    logMessage(LogLevel::Info, "ConnectionEngine Stopped");
}

int NetworkEngine::init()
{
#ifdef _WIN32
    // Start Winsock
    WSAData wsaData {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        logMessage(LogLevel::Fatal, "WSAStartup failed");
#endif

    // Connect the signals to slots
    acceptSignal.connect([](Connection client) {
        std::string ip = getIP(client);
        int port = getPort(client);
        logMessage(LogLevel::Info, "Client @ " + ip + ':' + std::to_string(port) + " connected");
    });
    disconnectSignal.connect([](Connection client) {
        std::string ip = getIP(client);
        int port = getPort(client);
        logMessage(LogLevel::Info, "Client @ " + ip + ':' + std::to_string(port) + " disconnected");
    });
    onReceiveKeepalive.connect([](Connection client) {
        std::string ip = getIP(client);
        int port = getPort(client);
        logMessage(LogLevel::Debug, "Received keepalive from client @ " + ip + ':' + std::to_string(port));
    });
    broadcastData.connect(&broadcast);


    auto acceptorCoroutine = ::acceptorCoroutineFunc();
    auto eventCoroutine = eventCoroutineFunc();

    logMessage(LogLevel::Info, "(ConnectionSubsystem) Started");
    return 0;
}

void NetworkEngine::broadcast(Connection sender, const std::string &data)
{
    //logMessage(LogLevel::DEBUG, "Attempting to broadcast message...");
    beforeBroadcastData(std::move(sender), data);

    auto connections = g_Registry.view<ClientConnection>();
    for (auto i : connections)
    {
        auto connection = static_cast<Connection>(i);
        if (sender == connection) continue; // Skip sender

        std::string ip  = getIP(sender);
        int port = getPort(sender);
        Message message(ip, port, data);

        // Attempt to send data
        if (sendData(connection, message.toString()))
            logMessage(LogLevel::Debug, "Sent message to client @ " + ip + ':' + std::to_string(port));
        else
            logMessage(LogLevel::Error, "Failed to send message to client @ " + ip + ':' + std::to_string(port));
    }

    afterBroadcastData(std::move(sender), data);
}

[[nodiscard]] Connection NetworkEngine::getServer()
{
    return g_ServerConnection;
}

[[nodiscard]] std::vector<Connection> NetworkEngine::clients()
{
    std::lock_guard lock(g_Mutex);
    std::vector<Connection> connections;
    for (auto clients = g_Registry.view<ClientConnection>(); auto client : clients)
        connections.emplace_back(static_cast<Connection>(client));
    return connections;
}

[[nodiscard]] size_t NetworkEngine::size()
{
    return g_Registry.view<ClientConnection>().size();
}

[[nodiscard]] bool NetworkEngine::empty()
{
    std::lock_guard lock(g_Mutex);
    return g_Registry.view<ClientConnection>().empty();
}

bool NetworkEngine::sendData(Connection sender, const std::string &data)
{
    if (!isValid(sender)) [[unlikely]] return false;

    std::lock_guard lock(g_Mutex);
    auto &socket = g_Registry.get<SocketInfo>(static_cast<entt::entity>(sender));
    auto &metrics = g_Registry.get<Metrics>(static_cast<entt::entity>(sender));

    // Don't do anything if the socket is invalid or data is empty
    if (socket.fd == -1 || data.empty()) [[unlikely]] return false;

    beforeSendData(std::move(sender));

    // Attempt to send data
    ssize_t bytesSent = send(socket.fd, data.c_str(), data.length(), 0);
    if (bytesSent == -1)
    {
#ifndef _WIN32
        logMessage(LogLevel::Error, "Error sending data: " + std::string(strerror(errno)));
#else
        logMessage(LogLevel::Error, "Error sending data: " + std::to_string(WSAGetLastError()));
#endif
        return false;
    }
    metrics.bytesSent += data.size();
    return true;
}


std::string NetworkEngine::receiveData(Connection connection)
{
    if (!isValid(connection) || !hasPendingData(connection)) [[unlikely]] return "";

    std::lock_guard lock(g_Mutex);
    auto &socket = g_Registry.get<SocketInfo>(static_cast<entt::entity>(connection));

    char buffer[1024] {};
    ssize_t bytesReceived = recv(socket.fd, buffer, sizeof(buffer), 0);

    if (bytesReceived == 0)
    {
        logMessage(LogLevel::Info, "Connection closed by peer");
        return "";
    }

    if (bytesReceived < 0)
    {
#ifndef _WIN32
        logMessage(LogLevel::Error, "Error receiving data: " + std::string(strerror(errno)));
#else
        logMessage(LogLevel::Error, "Error receiving data: " + std::to_string(WSAGetLastError()));
#endif
        return "";
    }

    // Update the last activity time if client connection
    if (connection != g_ServerConnection) [[likely]]
    {
        auto &clientInfo = g_Registry.get<ClientInfo>(static_cast<entt::entity>(connection));
        clientInfo.lastActivityTime = std::chrono::steady_clock::now();
    }

    auto &metrics = g_Registry.get<Metrics>(static_cast<entt::entity>(connection));
    metrics.bytesReceived += bytesReceived;
    return std::string(buffer, bytesReceived);
}

bool NetworkEngine::disconnect(Connection client)
{
    if (!isValid(client) || client == g_ServerConnection) [[unlikely]] return false;

    std::lock_guard lock(g_Mutex);

    // Do stuff before disconnection
    beforeDisconnect(std::move(client));

    // Remove connection from registry
    g_Registry.destroy(static_cast<entt::entity>(client));

    // Do stuff after disconnection
    afterDisconnect(std::move(client));
    return true;
}

[[nodiscard]] bool NetworkEngine::hasPendingData(Connection client)
{
    if (!isValid(client)) [[unlikely]] return false;

    std::lock_guard lock(g_Mutex);
    auto &socket = g_Registry.get<SocketInfo>(static_cast<entt::entity>(client));
    if (socket.fd == -1) return false;

    fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(socket.fd, &readFDs);

    timeval timeout = {0, 0};
    int result = select(socket.fd + 1, &readFDs, nullptr, nullptr, &timeout);

    return result > 0 && FD_ISSET(socket.fd, &readFDs);
}

[[nodiscard]] long NetworkEngine::getFD(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return false;
    return g_Registry.get<SocketInfo>(static_cast<entt::entity>(connection)).fd;
}

[[nodiscard]] std::string NetworkEngine::getIP(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return "";

    std::lock_guard lock(g_Mutex);
    auto &socket = g_Registry.get<SocketInfo>(static_cast<entt::entity>(connection));

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(socket.address.sin_addr), ipStr, INET_ADDRSTRLEN);
    return ipStr;
}

[[nodiscard]] int NetworkEngine::getPort(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return -1;

    std::lock_guard lock(g_Mutex);
    return ntohs(g_Registry.get<SocketInfo>(static_cast<entt::entity>(connection)).address.sin_port);
}

[[nodiscard]] bool NetworkEngine::isActive(Connection connection, int timeout)
{
    if (!isValid(connection)) [[unlikely]] return false;
    if (connection == g_ServerConnection) [[unlikely]] return true;

    auto &clientInfo = g_Registry.get<ClientInfo>(static_cast<entt::entity>(connection));

    auto currentTime = std::chrono::steady_clock::now();
    return (currentTime - clientInfo.lastActivityTime) > std::chrono::seconds(timeout);
}

[[nodiscard]] bool NetworkEngine::validate(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return false;

    std::lock_guard lock(g_Mutex);
    auto &socket = g_Registry.get<SocketInfo>(static_cast<entt::entity>(connection));
    if (socket.fd == -1) return false;

    char buffer[1];
    ssize_t result = recv(socket.fd, buffer, sizeof(buffer), MSG_PEEK);

    if (result < 0)
    {
#ifndef _WIN32
        if (errno == EAGAIN || errno == EWOULDBLOCK)
#else
        if (WSAGetLastError() == WSAEWOULDBLOCK)
#endif
            return true; // Non-blocking operation
        return false; // Other errors
    }
    return result != 0; // Valid if data is available
}

std::future<void> acceptorCoroutineFunc()
{
    return std::async(std::launch::async, []
    {
        while (server.isRunning())
        {
            if (acceptClient())
                // Notify the event thread
                m_ThreadCV.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        logMessage(LogLevel::Debug, "(ConnectionSubsystem) Stopped acceptor thread");
    });
}

std::future<void> eventCoroutineFunc() {
    return std::async(std::launch::async, []
    {
        while (server.isRunning())
        {
            std::unique_lock lock(m_ThreadMutex);
            m_ThreadCV.wait(lock, [] {
                return !server.isRunning() || NetworkEngine::size() > 1;
            });
            if (!server.isRunning()) break;
            lock.unlock();

            validateConnections();
            processConnections();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        logMessage(LogLevel::Debug, "(ConnectionSubsystem) Stopped event thread");
    });
}

void processConnectionsInternal(const std::function<bool(Connection)> &predicate)
{
    //logMessage(LogLevel::DEBUG, "Checking if connections need to be purged...");

    std::vector<Connection> connectionsToPurge;

    auto clients = g_Registry.view<ClientConnection>();
    for (auto client : clients)
        // If connection needs to be purged, purge it
            if (predicate(static_cast<Connection>(client)))
                connectionsToPurge.emplace_back(static_cast<Connection>(client));

    for (auto client : connectionsToPurge)
        NetworkEngine::disconnect(client);

    // Notify the event thread
    m_ThreadCV.notify_one();
}

void validateConnections()
{
    //logMessage(LogLevel::DEBUG, "Validating connections...");
    processConnectionsInternal([](Connection connection) {
        // Purge invalid or inactive connections
        return !(NetworkEngine::validate(connection) && NetworkEngine::isActive(connection, 30));
    });
}

void processConnections()
{
    //logMessage(LogLevel::DEBUG, "Processing connections...");

    // Check and process connections with data
    processConnectionsInternal([](Connection client) {
        std::string message = NetworkEngine::receiveData(client);

        // Purge if the message is empty ...
        if (!NetworkEngine::hasPendingData(client) && message.empty())
            return false;

        if (!message.empty())
        {
            // Process message if it's valid
            //logMessage(LogLevel::DEBUG, "Processing message...");
            MessageProcessor::processMessage(std::move(client), message);
            return false; // Don't purge this connection
        }
        // Purge if no message is received and there is no pending data
        return true;
    });
}

[[nodiscard]] Connection createConnection()
{
    std::lock_guard lock(g_Mutex);

    // 1. Check if a server connection has been created
    if (g_ServerConnection == -1) [[unlikely]] return -1;

    // 2. Check if the server is accepting connections (skip for now)


    // 3. Create entity
    auto connection = g_Registry.create();

    // 4. Create the socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0)
    {
        // Set to non-blocking mode, then return true if that works
#ifndef _WIN32
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            logMessage(LogLevel::Error, "Failed to to set socket to non-blocking mode");
            close(fd);
            return -1;
        }
#else
        u_long mode = 1; // 1 = non-blocking
        if (ioctlsocket(m_FD, FIONBIO, &mode) != 0)
        {
            logMessage(LogLevel::Error, "Failed to to set socket to non-blocking mode");
            close(fd);
            return -1;
        }
#endif
        // Enable keepalive
        int enableKeepalive = 1;
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepalive, sizeof(enableKeepalive));
    }
    else
    {
#ifndef _WIN32
        logMessage(LogLevel::Error, "Socket creation failed: " + std::string(strerror(errno)));
#else
        logMessage(LogLevel::Error, "Socket creation failed: " + std::to_string(WSAGetLastError()));
#endif
        return -1;
    }

    // 5. Add components to connection
    g_Registry.emplace<ClientConnection>(connection);
    g_Registry.emplace<ClientInfo>(connection);
    g_Registry.emplace<SocketInfo>(connection);
    g_Registry.emplace<Metrics>(connection);

    // 6. Bind destroy function to connection
    // if (!g_Registry.on_destroy<SocketInfo>().connect<onDestroyFunction>(connection))
    // {
    //     logMessage(LogLevel::Error, "Failed to bind destroy function to connection");
    //     g_Registry.destroy(connection);
    //     return -1;
    // }
    return static_cast<int>(connection);
}

// Any time a connection is destroyed, this is called (acts as a destructor)
void onDestroyFunction(const SocketInfo &socket)
{
    // Close socket
    if (socket.fd != -1) [[unlikely]]
#ifndef _WIN32
        close(socket.fd);
#else
    closesocket(socket.fd);
#endif
}

// Creates an object
inline int createSocket()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

// Creates a server sockaddr_in object
bool createServerAddress(sockaddr_in &address, int port)
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

// Accepts a client and returns a Connection ID if successful
Connection acceptClient()
{
    int clientFD = -1;
    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);
    clientFD = accept(clientFD, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
    if (clientFD == -1) return -1;

    auto clientID = g_Registry.create();

    g_Registry.emplace<ClientConnection>(clientID);
    g_Registry.emplace<ClientInfo>(clientID);
    g_Registry.emplace<SocketInfo>(clientID, clientFD, clientAddress);
    g_Registry.emplace<Metrics>(clientID);

    auto clientConnection = static_cast<Connection>(clientID);

    NetworkEngine::acceptSignal(std::move(clientConnection));
    return clientConnection;
}

// Checks if a connection is valid
inline bool isValid(Connection connection)
{
    return g_Registry.valid(static_cast<entt::entity>(connection));
}