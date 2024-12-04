//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "ServerModule.h"
#include "server/Signal.h"

/* Components */
struct ServerConnection;
struct ServerInfo;
struct ClientConnection;
struct ClientInfo;
struct SocketInfo;
struct Metrics;

class NetworkEngine : public ServerModule {
public signals:
    static Signal<> onStartup;
    static Signal<> onShutdown;

    static Signal<Connection> beforeAccept;
    static Signal<Connection> acceptSignal;
    static Signal<Connection> afterAccept;

    static Signal<Connection> beforeDisconnect;
    static Signal<Connection> disconnectSignal;
    static Signal<Connection> afterDisconnect;

    static Signal<Connection> beforeSendData;
    static Signal<Connection> sendDataSignal;
    static Signal<Connection> afterSendData;

    static Signal<Connection> beforeReceiveData;
    static Signal<Connection> receiveDataSignal;
    static Signal<Connection> afterReceiveData;

    static Signal<Connection, const std::string &> beforeBroadcastData;
    static Signal<Connection, const std::string &> broadcastData;
    static Signal<Connection, const std::string &> afterBroadcastData;

    static Signal<Connection> onReceiveKeepalive;

public slots:
    static void broadcast(Connection, const std::string &);

public:
    NetworkEngine();
    ~NetworkEngine() override;
    void init() override;
    void run() override;
    [[nodiscard]] std::vector<std::type_index> requiredDependencies() const override { return {}; }
    [[nodiscard]] std::vector<std::type_index> optionalDependencies() const override { return {}; }

    static Connection getServer();
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