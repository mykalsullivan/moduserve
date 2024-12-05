//
// Created by msullivan on 11/10/24.
//

#include "NetworkEngine.h"
#include "server/Server.h"
#include "common/Message.h"
#include <future>
#include <barrier>
#include <fcntl.h>
#include <entt/entt.hpp>

#include "server/modules/Logger.h"
#include "server/modules/MessageProcessor.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

/* Components */
struct ServerConnection {};
struct ServerInfo {
    int maxConnections = -1;
};

struct ClientConnection {};
struct ClientInfo {
    std::chrono::steady_clock::time_point lastActivityTime;
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

// Global variables
entt::registry g_ConnectionRegistry;
Connection g_ServerConnection;

std::mutex g_NetworkEngineMutex;
std::condition_variable g_NetworkEngineCV;
int port = 8000;

std::thread acceptorThread;
std::thread eventThread;

std::mutex g_NetworkThreadMutex;
std::barrier g_NetworkThreadBarrier(2);

// Forward declaration(s)
void acceptorThreadFunc(NetworkEngine &);
void eventThreadFunc(NetworkEngine &);
void validateConnections();
void processConnections();
void processConnectionsInternal(const std::function<bool(Connection)> &predicate);
void onDestroyFunction(const SocketInfo &socket);

Connection createConnection();
int createSocket();
bool createServerAddress(sockaddr_in &address, int port);
bool bindAddress(int serverFD, sockaddr_in serverAddress);
bool startListening(int serverFD);
bool acceptClient();

Connection entityToFD(entt::entity entity);
entt::entity fdToEntity(Connection connection);
bool isValid(Connection connection);

// Static signal definitions
Signal<> NetworkEngine::started;
Signal<> NetworkEngine::shutdown;
Signal<Connection> NetworkEngine::clientAccepted;
Signal<Connection> NetworkEngine::clientDisconnected;
Signal<Connection, const std::string &> NetworkEngine::sentData;
Signal<Connection, const std::string &> NetworkEngine::receivedData;
Signal<Connection, const std::string &> NetworkEngine::broadcastData;

Signal<Connection> NetworkEngine::receivedKeepalive;

// Static slots definitions
void NetworkEngine::onAccept(Connection connection)
{
    std::string ip = getIP(connection);
    std::string port = std::to_string(getPort(connection));
    std::string message = "Client @ " + ip + ':' + port + " connected";
    Logger::log(LogLevel::Info, message);
    broadcastData(std::move(connection), message);
}

void NetworkEngine::onDisconnect(Connection connection)
{
    std::string ip = getIP(connection);
    std::string port = std::to_string(getPort(connection));
    std::string message = "Client @ " + ip + ':' + port + " disconnected";
    Logger::log(LogLevel::Info, message);
    broadcastData(std::move(connection), message);
}

void NetworkEngine::onReceiveKeepalive(Connection connection)
{
    std::string ip = getIP(connection);
    std::string port = std::to_string(getPort(connection));
    Logger::log(LogLevel::Debug, "Receive keepalive from client @ " + ip + ':' + port);
}

NetworkEngine::~NetworkEngine()
{
    // Stop and join the event loop
    g_NetworkEngineCV.notify_all();

    if (acceptorThread.joinable()) acceptorThread.join();     // Wait for acceptor coroutine to finish
    if (eventThread.joinable()) eventThread.join();           // Wait for event coroutine to finish

#ifdef WIN32
    WSACleanup();
#endif

    // Cleanup connections
    auto clients = g_ConnectionRegistry.view<ClientConnection>();

    std::lock_guard lock(g_NetworkEngineMutex); // Protect access to m_Connections
    for (auto client : clients)
        disconnect(entityToFD(client));

    g_ConnectionRegistry.clear();

    Logger::log(LogLevel::Info, "ConnectionEngine Stopped");
}

void NetworkEngine::init()
{
#ifdef _WIN32
    // Start Winsock
    WSAData wsaData {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        Logger::log(LogLevel::Fatal, "WSAStartup failed");
#endif

    // 1. Create server connection
    auto serverConnection = g_ConnectionRegistry.create();
    g_ConnectionRegistry.emplace<ServerConnection>(serverConnection);
    g_ConnectionRegistry.emplace<ServerInfo>(serverConnection);

    // 2. Create file descriptor
    SocketInfo socket {};
    socket.fd = -1;
    if ((socket.fd = createSocket()) == -1)
    {
        Logger::log(LogLevel::Error, "Failed to create socket");
        g_ConnectionRegistry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    Logger::log(LogLevel::Debug, "Created server socket: " + std::to_string(socket.fd));

    // 3. Create server address
    int port = 8000;
    if (!createServerAddress(socket.address, port))
    {
        Logger::log(LogLevel::Error, "Failed to create address");
        g_ConnectionRegistry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    Logger::log(LogLevel::Debug, "Successfully created server address (listening on all interfaces)");

    // 4. Bind socket to address
    if (!bindAddress(socket.fd, socket.address))
    {
        Logger::log(LogLevel::Error, "Failed to bind address");
        g_ConnectionRegistry.destroy(serverConnection);
        g_ConnectionRegistry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    g_ConnectionRegistry.emplace<SocketInfo>(serverConnection);
    std::string ip = getIP(entityToFD(serverConnection));
    Logger::log(LogLevel::Debug, "Successfully bound to address (" + ip + ':' + std::to_string(port) + ')');

    // 5. Listen to incoming connections
    if (!startListening(socket.fd))
    {
        Logger::log(LogLevel::Error, "Cannot listen to incoming connections");
        g_ConnectionRegistry.destroy(serverConnection);
        exit(EXIT_FAILURE);
    }
    Logger::log(LogLevel::Info, "Listening for new connections on " + ip + ':' + std::to_string(port) + ')');

    g_ConnectionRegistry.emplace<Metrics>(serverConnection);

    g_ServerConnection = entityToFD(serverConnection);

    // Connect the signals to slots
    clientAccepted.connect(onAccept);
    clientDisconnected.connect(onDisconnect);
    broadcastData.connect(onReceiveBroadcast);
    receivedKeepalive.connect(onReceiveKeepalive);
    receivedData.connect(&MessageProcessor::processMessage);
    m_Initialized = true;
    m_Active = true;
}

void NetworkEngine::run()
{
    // Start acceptor thread
    acceptorThread = std::thread([this] {
        Logger::log(LogLevel::Debug, "Started acceptor thread...");
        while (isActive())
        {
            std::string message = "Waiting on clients to join... (currently: " + std::to_string(size()) + ')';
            Logger::log(LogLevel::Debug, message);
            if (acceptClient())
                g_NetworkEngineCV.notify_one(); // Notify the event thread
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        Logger::log(LogLevel::Debug, "(ConnectionSubsystem) Stopped acceptor thread");
    });

    // Start event thread
    eventThread = std::thread([this] {
        Logger::log(LogLevel::Debug, "Started event thread...");
        while (isActive())
        {
            Logger::log(LogLevel::Debug, "Waiting on clients to do stuff...");
            std::unique_lock lock(g_NetworkThreadMutex);
            g_NetworkEngineCV.wait(lock, [this] {
                return !isActive() || size() > 0;
            });
            if (!isActive()) break;
            lock.unlock();

            validateConnections();
            processConnections();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        Logger::log(LogLevel::Debug, "(ConnectionSubsystem) Stopped event thread");
    });
    Logger::log(LogLevel::Info, "(ConnectionSubsystem) Started");
}

void NetworkEngine::onReceiveBroadcast(Connection sender, const std::string &data)
{
    Logger::log(LogLevel::Debug, "Attempting to broadcast message...");

    auto clientEnts = g_ConnectionRegistry.view<ClientConnection>();
    for (auto clientEnt : clientEnts)
    {
        const auto client = entityToFD(clientEnt);
        if (sender == client) continue; // Skip sender

        std::string ip  = getIP(sender);
        int port = getPort(sender);
        Message message(ip, port, data);

        // Attempt to send data
        if (sendData(client, message.toString()))
            Logger::log(LogLevel::Debug, "Sent message to client @ " + ip + ':' + std::to_string(port));
        else
            Logger::log(LogLevel::Error, "Failed to send message to client @ " + ip + ':' + std::to_string(port));
    }
}

[[nodiscard]] Connection NetworkEngine::getServer()
{
    return g_ServerConnection;
}

[[nodiscard]] std::vector<Connection> NetworkEngine::clients()
{
    std::lock_guard lock(g_NetworkEngineMutex);
    std::vector<Connection> connections;
    for (auto client : g_ConnectionRegistry.view<ClientConnection>())
        connections.emplace_back(entityToFD(client));
    return connections;
}

[[nodiscard]] size_t NetworkEngine::size()
{
    return g_ConnectionRegistry.view<ClientConnection>().size();
}

[[nodiscard]] bool NetworkEngine::empty()
{
    std::lock_guard lock(g_NetworkEngineMutex);
    return g_ConnectionRegistry.view<ClientConnection>().empty();
}

bool NetworkEngine::sendData(Connection sender, const std::string &data)
{
    if (!isValid(sender)) [[unlikely]] return false;

    std::lock_guard lock(g_NetworkEngineMutex);
    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(sender));
    auto &metrics = g_ConnectionRegistry.get<Metrics>(fdToEntity(sender));

    // Don't do anything if the socket is invalid or data is empty
    if (socket.fd == -1 || data.empty()) [[unlikely]] return false;

    // Attempt to send data
    ssize_t bytesSent = send(socket.fd, data.c_str(), data.length(), 0);
    if (bytesSent == -1)
    {
#ifndef _WIN32
        Logger::log(LogLevel::Error, "Error sending data: " + std::string(strerror(errno)));
#else
        Logger::log(LogLevel::Error, "Error sending data: " + std::to_string(WSAGetLastError()));
#endif
        return false;
    }
    metrics.bytesSent += data.size();

    sentData(std::move(sender), data);
    return true;
}


std::string NetworkEngine::receiveData(Connection connection)
{
    if (!isValid(connection) || !hasPendingData(connection)) [[unlikely]] return "";

    std::lock_guard lock(g_NetworkEngineMutex);
    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection));

    char buffer[1024] {};
    ssize_t bytesReceived = recv(socket.fd, buffer, sizeof(buffer), 0);

    if (bytesReceived == 0)
    {
        Logger::log(LogLevel::Info, "Connection closed by peer");
        return "";
    }

    if (bytesReceived < 0)
    {
#ifndef _WIN32
        Logger::log(LogLevel::Error, "Error receiving data: " + std::string(strerror(errno)));
#else
        Logger::log(LogLevel::Error, "Error receiving data: " + std::to_string(WSAGetLastError()));
#endif
        return "";
    }

    // Update the last activity time if client connection
    if (connection != g_ServerConnection) [[likely]]
    {
        auto &clientInfo = g_ConnectionRegistry.get<ClientInfo>(fdToEntity(connection));
        clientInfo.lastActivityTime = std::chrono::steady_clock::now();
    }

    auto &metrics = g_ConnectionRegistry.get<Metrics>(fdToEntity(connection));
    metrics.bytesReceived += bytesReceived;

    receivedData(std::move(connection), buffer);
    return std::string(buffer, bytesReceived);
}

bool NetworkEngine::disconnect(Connection client)
{
    if (!isValid(client) || client == g_ServerConnection) [[unlikely]] return false;

    std::lock_guard lock(g_NetworkEngineMutex);

    // Remove connection from registry
    clientDisconnected(std::move(client));
    g_ConnectionRegistry.destroy(fdToEntity(client));
    return true;
}

[[nodiscard]] bool NetworkEngine::hasPendingData(Connection client)
{
    if (!isValid(client)) [[unlikely]] return false;

    std::lock_guard lock(g_NetworkEngineMutex);
    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(client));
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
    return g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection)).fd;
}

[[nodiscard]] std::string NetworkEngine::getIP(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return "";

    std::lock_guard lock(g_NetworkEngineMutex);
    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection));

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(socket.address.sin_addr), ipStr, INET_ADDRSTRLEN);
    return ipStr;
}

[[nodiscard]] int NetworkEngine::getPort(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return -1;

    std::lock_guard lock(g_NetworkEngineMutex);
    return ntohs(g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection)).address.sin_port);
}

[[nodiscard]] bool NetworkEngine::isActiveConnection(Connection connection, int timeout)
{
    if (!isValid(connection)) [[unlikely]] return false;
    if (connection == g_ServerConnection) [[unlikely]] return true;

    auto &clientInfo = g_ConnectionRegistry.get<ClientInfo>(fdToEntity(connection));

    auto currentTime = std::chrono::steady_clock::now();
    return (currentTime - clientInfo.lastActivityTime) > std::chrono::seconds(timeout);
}

[[nodiscard]] bool NetworkEngine::isValidConnection(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return false;

    std::lock_guard lock(g_NetworkEngineMutex);
    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection));
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

void processConnectionsInternal(const std::function<bool(Connection)> &predicate)
{
    //Logger::log(LogLevel::DEBUG, "Checking if connections need to be purged...");

    std::vector<Connection> connectionsToPurge;

    auto clients = g_ConnectionRegistry.view<ClientConnection>();
    for (auto client : clients)
        if (predicate(entityToFD(client)))
            connectionsToPurge.emplace_back(entityToFD(client));

    for (auto client : connectionsToPurge)
        NetworkEngine::disconnect(client);

    // Notify the event thread
    g_NetworkEngineCV.notify_one();
}

void validateConnections()
{
    //Logger::log(LogLevel::DEBUG, "Validating connections...");
    processConnectionsInternal([](Connection connection) {
        // Purge invalid or inactive connections
        return !(NetworkEngine::isValidConnection(connection) && NetworkEngine::isActiveConnection(connection, 30));
    });
}

void processConnections()
{
    //Logger::log(LogLevel::DEBUG, "Processing connections...");

    // Check and process connections with data
    processConnectionsInternal([](Connection client) {
        std::string message = NetworkEngine::receiveData(client);

        // Purge if the message is empty ...
        if (!NetworkEngine::hasPendingData(client) && message.empty())
            return false;

        if (!message.empty())
        {
            // Process message if it's valid
            //Logger::log(LogLevel::DEBUG, "Processing message...");
            MessageProcessor::processMessage(std::move(client), message);
            return false; // Don't purge this connection
        }
        // Purge if no message is received and there is no pending data
        return true;
    });
}

[[nodiscard]] Connection createConnection()
{
    std::lock_guard lock(g_NetworkEngineMutex);

    // 1. Check if a server connection has been created
    if (g_ServerConnection == -1) [[unlikely]] return -1;

    // 2. Check if the server is accepting connections (skip for now)


    // 3. Create entity
    auto connection = g_ConnectionRegistry.create();

    // 4. Create the socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0)
    {
        // Set to non-blocking mode, then return true if that works
#ifndef _WIN32
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            Logger::log(LogLevel::Error, "Failed to to set socket to non-blocking mode");
            close(fd);
            return -1;
        }
#else
        u_long mode = 1; // 1 = non-blocking
        if (ioctlsocket(m_FD, FIONBIO, &mode) != 0)
        {
            Logger::log(LogLevel::Error, "Failed to to set socket to non-blocking mode");
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
        Logger::log(LogLevel::Error, "Socket creation failed: " + std::string(strerror(errno)));
#else
        Logger::log(LogLevel::Error, "Socket creation failed: " + std::to_string(WSAGetLastError()));
#endif
        return -1;
    }

    // 5. Add components to connection
    g_ConnectionRegistry.emplace<ClientConnection>(connection);
    g_ConnectionRegistry.emplace<ClientInfo>(connection);
    g_ConnectionRegistry.emplace<SocketInfo>(connection);
    g_ConnectionRegistry.emplace<Metrics>(connection);

    // 6. Bind destroy function to connection
    // if (!g_Registry.on_destroy<SocketInfo>().connect<onDestroyFunction>(connection))
    // {
    //     Logger::log(LogLevel::Error, "Failed to bind destroy function to connection");
    //     g_Registry.destroy(connection);
    //     return -1;
    // }
    return entityToFD(connection);
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
bool acceptClient()
{
    std::string message = "Waiting on new connections...";
    Logger::log(LogLevel::Debug, message);

    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);

    auto serverFD = NetworkEngine::getFD(g_ServerConnection);
    int clientFD = accept(serverFD, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength); // TODO issue here
    if (clientFD == -1) return false;

    message = "Attempted to accept a new connection...";
    Logger::log(LogLevel::Debug, message);

    const auto client = g_ConnectionRegistry.create();

    g_ConnectionRegistry.emplace<ClientConnection>(client);
    g_ConnectionRegistry.emplace<ClientInfo>(client);
    g_ConnectionRegistry.emplace<SocketInfo>(client, clientFD, clientAddress);
    g_ConnectionRegistry.emplace<Metrics>(client);

    NetworkEngine::clientAccepted(entityToFD(client));
    return true;
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
    return g_ConnectionRegistry.valid(fdToEntity(connection));
}