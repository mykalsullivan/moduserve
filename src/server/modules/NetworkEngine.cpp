//
// Created by msullivan on 11/10/24.
//

#include "NetworkEngine.h"
#include "server/Server.h"
#include "common/Message.h"
#include <sstream>
#include <future>
#include <barrier>
#include <fcntl.h>
#include <entt/entt.hpp>

#include "server/modules/Logger.h"

// Temp
#include "server/optional_modules/BFModule.h"

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

// Global variables
namespace {
    entt::registry g_ConnectionRegistry;
    Connection g_ServerConnection;
    int g_Port;

    std::mutex g_NetworkEngineMutex;
    std::condition_variable g_NetworkEngineCV;

    std::thread acceptorThread;
    std::thread eventThread;

    std::mutex g_NetworkThreadMutex;
    std::barrier g_NetworkThreadBarrier(2);
}

// Forward declaration(s)
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

void NetworkEngine::onSentData(Connection connection, const std::string &data)
{
    std::string ip = getIP(connection);
    std::string port = std::to_string(getPort(connection));
    std::string message = "Sent \"" + data + "\" to client @ " + ip + ':' + port;
    Logger::log(LogLevel::Debug, message);
}

void NetworkEngine::onReceivedData(Connection connection, const std::string &data)
{
    std::string ip = getIP(connection);
    std::string port = std::to_string(getPort(connection));
    std::string message = "Client @ " + ip + ':' + port + ": \"" + data + '"';
    Logger::log(LogLevel::Debug, message);
}

void NetworkEngine::processMessage(Connection connection, const std::string &data)
{
    std::istringstream ss(data);
    std::string arg;
    std::vector<std::string> args;

    while (ss >> arg) args.emplace_back(arg);

    if (args[0] == "EXEC")
    {
        if (args.size() > 2)
        {
            // This should parse the command arguments and pass them into the command
            if (args[1] == "bf")
            {
                if (args.size() > 3)
                {
                    if (args[2] == "execute")
                        BFModule::receivedCode(std::move(connection), args[3]);
                    else if (args[2] == "retrieve_pointer_address")
                        BFModule::retrieveCurrentPointerAddress(connection);
                    else if (args[2] == "retrieve_pointer_value")
                        BFModule::retrieveCurrentPointerValue(connection);
                    else if (args[2] == "retrieve_tape_values")
                        BFModule::retrieveTapeValues(connection, std::stoi(args[3]), std::stoi(args[4]));
                }
            }
            else
                // Print usage information for the command
                sendData(std::move(connection), "Missing command arguments");
        }
        else
            sendData(std::move(connection), "Please specify a command");
    }
    else if (args[0] == "SAY")
    {
        if (args.size() > 1)
        {

        }
        else
            Logger::log(LogLevel::Debug, "Invalid number of arguments");
    }
    else if (args[0] == "SAY_ALL")
    {
        std::string message;

        for (auto i = 0; i < args.size(); i++)
        {
            if (i == args.size()) continue;
            message += args[i] + ' ';
        }
        broadcastData(std::move(connection), message);
    }
    else if (data == "KEEPALIVE")
        receivedKeepalive(std::move(connection));
    else
    {
        std::string message = "Invalid command \"" + data + '"';
        sendData(std::move(connection), message);
    }
}

void NetworkEngine::onReceivedKeepalive(Connection connection)
{
    std::string ip = getIP(connection);
    std::string port = std::to_string(getPort(connection));
    Logger::log(LogLevel::Debug, "Received keepalive from client @ " + ip + ':' + port);
}

NetworkEngine::NetworkEngine(int port = 7035)
{
    g_Port = port;
}

NetworkEngine::~NetworkEngine()
{
    // Stop and join the event loop
    m_Active = false;
    g_NetworkEngineCV.notify_all();

    if (acceptorThread.joinable()) acceptorThread.join();     // Wait for acceptor coroutine to finish
    if (eventThread.joinable()) eventThread.join();           // Wait for event coroutine to finish

#ifdef WIN32
    WSACleanup();
#endif

    // Cleanup connections
    auto clients = g_ConnectionRegistry.view<ClientConnection>();
    for (auto client : clients)
        disconnect(entityToFD(client));

    g_ConnectionRegistry.clear();

    Logger::log(LogLevel::Info, "Network engine Stopped");
}

void NetworkEngine::init()
{
#ifdef _WIN32
    // Start Winsock
    WSAData wsaData {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        Logger::log(LogLevel::Fatal, "WSAStartup failed");
#endif

    // Create the server connection
    Connection serverFD = createConnection(true, g_Port);
    if (serverFD == -1)
    {
        Logger::log(LogLevel::Fatal, "Failed to create server connection");
        exit(EXIT_FAILURE);
    }
    Logger::log(LogLevel::Info, "Created server socket: " + std::to_string(serverFD));

    // Get the server entity from the file descriptor
    auto serverEntity = fdToEntity(serverFD);

    // Verify server entity has been set up correctly
    if (!g_ConnectionRegistry.valid(serverEntity))
    {
        Logger::log(LogLevel::Fatal, "Server entity creation failed");
        exit(EXIT_FAILURE);
    }

    // Log server initialization details
    std::string ip = getIP(serverFD);
    Logger::log(LogLevel::Info, "Server initialized and listening on " + ip + ':' + std::to_string(g_Port));

    // Store server connection globally
    g_ServerConnection = serverFD;

    // Connect the signals to slots
    clientAccepted.connect(onAccept);
    clientDisconnected.connect(onDisconnect);
    sentData.connect(onSentData);
    receivedData.connect(onReceivedData);
    receivedData.connect(processMessage);
    receivedKeepalive.connect(onReceivedKeepalive);
    broadcastData.connect(onReceivedBroadcast);

    m_Initialized = true;
    m_Active = true;
}

void NetworkEngine::run()
{
    // Start acceptor thread
    acceptorThread = std::thread([this]
    {
        g_NetworkThreadBarrier.arrive_and_wait();

        Logger::log(LogLevel::Info, "Started client acceptor thread");
        while (isActive())
        {
            if (acceptClient())
                g_NetworkEngineCV.notify_one(); // Notify the event thread
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    // Start event thread
    eventThread = std::thread([this]
    {
        g_NetworkThreadBarrier.arrive_and_wait();

        Logger::log(LogLevel::Info, "Started event thread");
        while (isActive())
        {
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
        Logger::log(LogLevel::Info, "Stopped event thread");
    });
}

void NetworkEngine::onReceivedBroadcast(Connection sender, const std::string &data)
{
    // Construct message
    std::string ip  = getIP(sender);
    int port = getPort(sender);
    Message message(ip, port, data);

    // Send message to all client other than the sender
    auto clientEnts = g_ConnectionRegistry.view<ClientConnection>();
    for (auto clientEnt : clientEnts)
    {
        auto client = entityToFD(clientEnt);
        if (sender == client) continue; // Skip sender

        // Attempt to send data
        sendData(client, message.content());
    }
    Logger::log(LogLevel::Info, "Broadcast message from client @ " + ip + ':' + std::to_string(port) +
                                 ": \"" + data + '"');
}

[[nodiscard]] Connection NetworkEngine::getServer()
{
    return g_ServerConnection;
}

[[nodiscard]] std::vector<Connection> NetworkEngine::clients()
{
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
    return g_ConnectionRegistry.view<ClientConnection>().empty();
}

bool NetworkEngine::sendData(Connection sender, const std::string &data)
{
    if (!isValid(sender)) [[unlikely]] return false;

    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(sender));
    auto &metrics = g_ConnectionRegistry.get<Metrics>(fdToEntity(sender));

    // Don't do anything if the socket is invalid or data is empty
    if (socket.fd == -1 || data.empty()) [[unlikely]] return false;

    // Attempt to send data
    std::unique_lock lock(g_NetworkEngineMutex);
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
    lock.unlock();

    sentData(std::move(sender), data);
    return true;
}

std::string NetworkEngine::receiveData(Connection connection)
{
    if (!isValid(connection) || !hasPendingData(connection)) [[unlikely]] return "";

    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection));

    std::unique_lock lock(g_NetworkEngineMutex);
    char buffer[1024] {};
    ssize_t bytesReceived = recv(socket.fd, buffer, sizeof(buffer), 0);
    lock.unlock();

    if (bytesReceived == 0)
    {
        Logger::log(LogLevel::Debug, "Connection closed by peer");
        disconnect(connection);
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
    lock.lock();
    if (connection != g_ServerConnection) [[likely]]
    {
        auto &clientInfo = g_ConnectionRegistry.get<ClientInfo>(fdToEntity(connection));
        clientInfo.lastActivityTime = std::chrono::steady_clock::now();
    }

    auto &metrics = g_ConnectionRegistry.get<Metrics>(fdToEntity(connection));
    metrics.bytesReceived += bytesReceived;
    lock.unlock();

    receivedData(std::move(connection), buffer);
    return std::string(buffer, bytesReceived);
}

bool NetworkEngine::disconnect(Connection client)
{
    if (!isValid(client) || client == g_ServerConnection) [[unlikely]] return false;

    // Remove connection from registry
    clientDisconnected(std::move(client));

    // Close client socket
    std::lock_guard lock(g_NetworkEngineMutex);
    if (getFD(client) != -1) [[unlikely]]
#ifndef _WIN32
        close(getFD(client));
#else
        closesocket(socket.fd);
#endif

    g_ConnectionRegistry.destroy(fdToEntity(client));
    return true;
}

[[nodiscard]] bool NetworkEngine::hasPendingData(Connection client)
{
    if (!isValid(client)) [[unlikely]] return false;

    auto entity = fdToEntity(client);
    if (entity == entt::null)
    {
        Logger::log(LogLevel::Error, "Invalid entity for client connection " + client);
        return false;
    }

    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(client));
    if (socket.fd == -1)
    {
        Logger::log(LogLevel::Error, "Invalid file descriptor for client " + client);
        return false;
    }

    fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(socket.fd, &readFDs);

    timeval timeout = {0, 0};
    int result = select(socket.fd + 1, &readFDs, nullptr, nullptr, &timeout);

    if (result > 0 && FD_ISSET(socket.fd, &readFDs)) return true;
    return false;
}

[[nodiscard]] long NetworkEngine::getFD(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return false;
    return g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection)).fd;
}

[[nodiscard]] std::string NetworkEngine::getIP(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return "";

    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection));
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(socket.address.sin_addr), ipStr, INET_ADDRSTRLEN);
    return ipStr;
}

[[nodiscard]] int NetworkEngine::getPort(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return -1;
    return ntohs(g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection)).address.sin_port);
}

[[nodiscard]] bool NetworkEngine::isActiveConnection(Connection connection, int timeout)
{
    if (!isValid(connection)) [[unlikely]] return false;
    if (connection == g_ServerConnection) [[unlikely]] return true;

    auto &clientInfo = g_ConnectionRegistry.get<ClientInfo>(fdToEntity(connection));

    auto currentTime = std::chrono::steady_clock::now();
    bool isActive = (currentTime - clientInfo.lastActivityTime) < std::chrono::seconds(timeout);
    return isActive;
}

[[nodiscard]] bool NetworkEngine::isValidConnection(Connection connection)
{
    if (!isValid(connection)) [[unlikely]] return false;

    auto &socket = g_ConnectionRegistry.get<SocketInfo>(fdToEntity(connection));
    if (socket.fd == -1) return false;

    char buffer[1];
    ssize_t result = recv(socket.fd, buffer, sizeof(buffer), MSG_PEEK);

    if (result < 0) {
#ifndef _WIN32
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
#else
        if (WSAGetLastError() == WSAEWOULDBLOCK)
#endif
            return true; // Non-blocking operation
        }
        return false; // Other errors
    }
    return true; // Valid if data is available
}

void processConnectionsInternal(const std::function<bool(Connection)> &predicate)
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

void validateConnections()
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

void processConnections()
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

[[nodiscard]] Connection createConnection(bool isServer, int port = 0)
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

[[nodiscard]] entt::entity createConnectionEntity(int fd, sockaddr_in clientAddress, bool isServer = false)
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
bool acceptClient()
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

[[nodiscard]] int createAndConfigureSocket(bool isServer, int port = 0)
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

// Returns the file descriptor of a specified entity
inline Connection entityToFD(entt::entity entity)
{
    return g_ConnectionRegistry.get<SocketInfo>(entity).fd;
}

entt::entity fdToEntity(Connection connection)
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