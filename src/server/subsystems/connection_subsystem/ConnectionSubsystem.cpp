//
// Created by msullivan on 11/10/24.
//

#include "ConnectionSubsystem.h"
#include "ConnectionManager.h"
#include "server/Server.h"
#include "ServerConnection.h"
#include "common/Logger.h"
#include <barrier>

static ConnectionManager connectionManager;

ConnectionSubsystem::ConnectionSubsystem() : m_ThreadBarrier(2)
{}

ConnectionSubsystem::~ConnectionSubsystem()
{
    // Stop and join the event loop
    m_ThreadCV.notify_all();
    if (m_EventThread.joinable()) m_EventThread.join();

    // Join the acceptor loop
    if (m_AcceptorThread.joinable()) m_AcceptorThread.join();

    logMessage(LogLevel::INFO, "(ConnectionSubsystem) Stopped");
}

int ConnectionSubsystem::init()
{
    // Connect the signals to slots
    connectSignal(onConnect, &ConnectionSubsystem::onConnectFunction);
    connectSignal(onDisconnect, &ConnectionSubsystem::onDisconnectFunction);
    connectSignal(onBroadcast, &ConnectionSubsystem::broadcastMessage);

    // Register signals with the SignalManager
    REGISTER_SIGNAL("onConnect", onConnect);
    REGISTER_SIGNAL("onDisconnect", onDisconnect);
    REGISTER_SIGNAL("onBroadcast", onBroadcast);

    // Define and start acceptor thread
    m_AcceptorThread = std::thread([this] {
        // Wait for all threads to be created
        m_ThreadBarrier.arrive_and_wait();

        logMessage(LogLevel::DEBUG, "(ConnectionSubsystem) Started acceptor thread");
        while (server.isRunning())
        {
            if (auto client = reinterpret_cast<ServerConnection *>(connectionManager[connectionManager.serverFD()])->acceptClient())
            {
                if (connectionManager.add(*client))
                {
                    // Notify the event thread
                    m_ThreadCV.notify_one();
                    onBroadcast.emit(*connectionManager[connectionManager.serverFD()],
                                     "Client @ " + client->getIP() + ':' + std::to_string(client->getPort()) +
                                     " connected");
                }
                else
                {
                    logMessage(LogLevel::ERROR, "Failed to add client connection");
                    delete client;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        logMessage(LogLevel::DEBUG, "(ConnectionSubsystem) Stopped acceptor thread");
    });

    // Define and start event thread
    m_EventThread = std::thread([this] {
        // Wait for all threads to be created
        m_ThreadBarrier.arrive_and_wait();

        logMessage(LogLevel::DEBUG, "(ConnectionSubsystem) Started event thread");
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
        logMessage(LogLevel::DEBUG, "(ConnectionSubsystem) Stopped event thread");
    });

    logMessage(LogLevel::INFO, "(ConnectionSubsystem) Started");
    return 0;
}

void ConnectionSubsystem::processConnectionsInternal(const std::function<bool(Connection *)>& connectionPredicate)
{
    //logMessage(LogLevel::DEBUG, "Checking if connections need to be purged...");

    std::vector<int> connectionsToPurge;
    for (auto &it : connectionManager)
    {
        int fd = it.first;
        if (fd == connectionManager.serverFD()) continue; // Skip server connection
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

void ConnectionSubsystem::validateConnections()
{
    //logMessage(LogLevel::DEBUG, "Validating connections...");

    processConnectionsInternal([](Connection *connection) {
        return !(connection && connection->isValid() && !connection->isInactive(30)); // Purge invalid or inactive connections
    });
}

void ConnectionSubsystem::processConnections()
{
    //logMessage(LogLevel::DEBUG, "Processing connections...");

    // Check and process connections with data
    processConnectionsInternal([](Connection* connection) {
        std::string message = connection->receiveData();

        // Purge if the message is empty ...
        if (!connection->hasPendingData() && message.empty())
            return false;

        if (!message.empty())
        {
            // Process message if it's valid
            //logMessage(LogLevel::DEBUG, "Processing message...");
            auto signal = GET_SIGNAL("onReceive", const Connection &, const std::string &);
            if (signal)signal->emit(*connection, message);

            return false; // Don't purge this connection
        }
        // Purge if no message is received and there is no pending data
        return true;
    });
}

void ConnectionSubsystem::onConnectFunction(const Connection &connection)
{
    logMessage(LogLevel::INFO, "Client @ " + connection.getIP() + ':' + std::to_string(connection.getPort()) + " connected");
}

void ConnectionSubsystem::onDisconnectFunction(const Connection &connection)
{
    logMessage(LogLevel::INFO, "Client @ " + connection.getIP() + ':' + std::to_string(connection.getPort()) + " disconnected");
}

void ConnectionSubsystem::broadcastMessage(const Connection &sender, const std::string &message)
{
    //logMessage(LogLevel::DEBUG, "Attempting to broadcast message...");
    for (auto &[fd, connection] : connectionManager)
    {
        // Skip server and sender
        if (fd == connectionManager.serverFD() || fd == sender.getFD() || !connection) continue;

        // Attempt to send message
        if (connection->sendData(message))
            logMessage(LogLevel::DEBUG,
                       "Sent message to client @ " +
                       connection->getIP() + ':' +
                       std::to_string(connection->getPort()));
        else
            logMessage(LogLevel::ERROR,
                       "Failed to send message to client @ " +
                       connection->getIP() + ':' +
                       std::to_string(connection->getPort()));
    }
}
