//
// Created by msullivan on 11/10/24.
//

#include "ConnectionModule.h"
#include "ConnectionManager.h"
#include "server/Server.h"
#include "common/Logger.h"
#include "common/Message.h"
#include <barrier>

#ifdef WIN32
#include <winsock2.h>
#endif

static ConnectionManager connectionManager;
int port = 8000;

ConnectionModule::ConnectionModule() : m_ThreadBarrier(2)
{
    // 1. Create server connection
    auto serverConnection = new ServerConnection();

    // 2. Create socket
    if (!serverConnection->createSocket())
    {
        logMessage(LogLevel::Error, "Failed to create socket");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Debug, "Created server socket: " + std::to_string(serverConnection->fd()));

    // 3. Create server address
    if (!serverConnection->createAddress(port))
    {
        logMessage(LogLevel::Error, "Failed to create address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Debug, "Successfully created server address (listening on all interfaces)");

    // 4. Bind socket to address
    if (!serverConnection->bindAddress())
    {
        logMessage(LogLevel::Error, "Failed to bind address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Debug, "Successfully bound to address (" + serverConnection->ip() + ':' + std::to_string(
            serverConnection->port()) + ')');

    // 5. Listen to incoming connections
    if (!serverConnection->startListening())
    {
        logMessage(LogLevel::Error, "Cannot listen to incoming connections");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    logMessage(LogLevel::Info, "Listening for new connections on " + serverConnection->ip() + ':' +
                               std::to_string(serverConnection->port()) + ')');

    connectionManager.add(*serverConnection);
    m_ServerFD = serverConnection->fd();
}

ConnectionModule::~ConnectionModule()
{
    // Stop and join the event loop
    m_ThreadCV.notify_all();
    if (m_EventThread.joinable()) m_EventThread.join();

    // Join the acceptor loop
    if (m_AcceptorThread.joinable()) m_AcceptorThread.join();

#ifdef WIN32
    WSACleanup();
#endif
    logMessage(LogLevel::Info, "(ConnectionSubsystem) Stopped");
}

int ConnectionModule::init()
{
#ifdef WIN32
    // Start Winsock
    WSAData wsaData {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        logMessage(LogLevel::Fatal, "WSAStartup failed");
#endif

    // Connect the signals to slots
    connectSignal(onConnect, &ConnectionModule::onConnectFunction);
    connectSignal(onDisconnect, &ConnectionModule::onDisconnectFunction);
    connectSignal(onBroadcast, &ConnectionModule::broadcastMessage);

    // Register signals with the SignalManager
    REGISTER_SIGNAL("onConnect", onConnect);
    REGISTER_SIGNAL("onDisconnect", onDisconnect);
    REGISTER_SIGNAL("onBroadcast", onBroadcast);

    // Define and start acceptor thread
    m_AcceptorThread = std::thread([this] {
        // Wait for all threads to be created
        m_ThreadBarrier.arrive_and_wait();

        logMessage(LogLevel::Debug, "(ConnectionSubsystem) Started acceptor thread");
        while (server.isRunning())
        {
            if (auto client = reinterpret_cast<ServerConnection *>(connectionManager[m_ServerFD])->acceptClient())
            {
                if (connectionManager.add(*client))
                {
                    // Notify the event thread
                    m_ThreadCV.notify_one();
                    onBroadcast.emit(*connectionManager[m_ServerFD],
                                     "Client @ " + client->ip() + ':' + std::to_string(client->port()) +
                                     " connected");
                }
                else
                {
                    logMessage(LogLevel::Error, "Failed to add client connection");
                    delete client;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        logMessage(LogLevel::Debug, "(ConnectionSubsystem) Stopped acceptor thread");
    });

    // Define and start event thread
    m_EventThread = std::thread([this] {
        // Wait for all threads to be created
        m_ThreadBarrier.arrive_and_wait();

        logMessage(LogLevel::Debug, "(ConnectionSubsystem) Started event thread");
        while (server.isRunning())
        {
            std::unique_lock lock(m_ThreadMutex);
            m_ThreadCV.wait(lock, [this] {
                return !server.isRunning() || connectionManager.size() > 1;
            });
            if (!server.isRunning()) break;
            lock.unlock();

            validateConnections();
            processConnections();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        logMessage(LogLevel::Debug, "(ConnectionSubsystem) Stopped event thread");
    });

    logMessage(LogLevel::Info, "(ConnectionSubsystem) Started");
    return 0;
}

void ConnectionModule::processConnectionsInternal(const std::function<bool(OldConnection *)>& connectionPredicate)
{
    //logMessage(LogLevel::DEBUG, "Checking if connections need to be purged...");

    std::vector<int> connectionsToPurge;
    for (auto &it : connectionManager)
    {
        int fd = it.first;
        if (fd == m_ServerFD) continue; // Skip server connection
        auto connection = it.second;

        // If connection needs to be purged, delete it
        if (connectionPredicate(connection))
            connectionsToPurge.emplace_back(fd);
    }

    // Purge connections
    for (auto i : connectionsToPurge)
        connectionManager.remove(i);

    // Notify the event thread
    m_ThreadCV.notify_one();
}

void ConnectionModule::validateConnections()
{
    //logMessage(LogLevel::DEBUG, "Validating connections...");

    processConnectionsInternal([](OldConnection *connection) {
        return !(connection && connection->isValid() && !connection->isInactive(30)); // Purge invalid or inactive connections
    });
}

void ConnectionModule::processConnections()
{
    //logMessage(LogLevel::DEBUG, "Processing connections...");

    // Check and process connections with data
    processConnectionsInternal([](OldConnection* connection) {
        std::string message = connection->receiveData();

        // Purge if the message is empty ...
        if (!connection->hasPendingData() && message.empty())
            return false;

        if (!message.empty())
        {
            // Process message if it's valid
            //logMessage(LogLevel::DEBUG, "Processing message...");
            auto signal = GET_SIGNAL("onReceive", const OldConnection &, const std::string &);
            if (signal)signal->emit(*connection, message);

            return false; // Don't purge this connection
        }
        // Purge if no message is received and there is no pending data
        return true;
    });
}

void ConnectionModule::onConnectFunction(const OldConnection &connection)
{
    logMessage(LogLevel::Info, "Client @ " + connection.ip() + ':' + std::to_string(connection.port()) + " connected");
}

void ConnectionModule::onDisconnectFunction(const OldConnection &connection)
{
    logMessage(LogLevel::Info, "Client @ " + connection.ip() + ':' + std::to_string(connection.port()) + " disconnected");
}

void ConnectionModule::broadcastMessage(const OldConnection &sender, const std::string &data)
{
    //logMessage(LogLevel::DEBUG, "Attempting to broadcast message...");
    for (auto &[fd, connection] : connectionManager)
    {
        // Skip server and sender
        if (fd == m_ServerFD || fd == sender.fd() || !connection) continue;

        Message message(sender.ip(), sender.port(), data);

        // Attempt to send data
        if (connection->sendData(message.toString()))
            logMessage(LogLevel::Debug,
                       "Sent message to client @ " +
                               connection->ip() + ':' +
                       std::to_string(connection->port()));
        else
            logMessage(LogLevel::Error,
                       "Failed to send message to client @ " +
                               connection->ip() + ':' +
                       std::to_string(connection->port()));
    }
}
