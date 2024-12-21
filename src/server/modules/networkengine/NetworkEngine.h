//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "server/modules/ServerModule.h"

/* Components */
struct ServerConnection;
struct ServerInfo;
struct ClientConnection;
struct ClientInfo;
struct SocketInfo;
struct Metrics;

class NetworkEngine : public ServerBackgroundService {
public signals:
    static Signal<> started;
    static Signal<> shutdown;
    static Signal<Connection> clientAccepted;
    static Signal<Connection> clientDisconnected;
    static Signal<Connection, const std::string &> sentData;
    static Signal<Connection, const std::string &> receivedData;
    static Signal<Connection, const std::string &> broadcastData;
    static Signal<Connection> receivedKeepalive;

public slots:
    static void onAccept(Connection);
    static void onDisconnect(Connection);
    static void onSentData(Connection, const std::string &);
    static void onReceivedData(Connection, const std::string &);
    static void onReceivedBroadcast(Connection, const std::string &);
    static void onReceivedKeepalive(Connection);
    static void processMessage(Connection connection, const std::string &);

public:
  	explicit NetworkEngine(int port = 7035);
    ~NetworkEngine() override;
    void init() override;
    void run() override;
    [[nodiscard]] std::vector<std::type_index> requiredDependencies() const override { return {}; }
    [[nodiscard]] std::vector<std::type_index> optionalDependencies() const override { return {}; }

    static Connection getServerConnection();
    static std::vector<Connection> clients();
    static size_t size();
    static bool empty();

    static bool disconnect(Connection);
    static bool sendData(Connection sender, const std::string &);
    static std::string receiveData(Connection);
    static bool hasPendingData(Connection);

    static long getFD(Connection);
    static std::string getIP(Connection);
    static int getPort(Connection);

    static bool isActiveConnection(Connection, int timeout);
    static bool isValidConnection(Connection);
};