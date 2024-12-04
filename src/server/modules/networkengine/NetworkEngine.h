//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../ServerModule.h"
#include "Connection.h"
#include "server/ServerSignal.h"

#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

/* Components */
struct ServerConnection {};
struct ClientConnection {};

struct ServerInfo {
    int maxConnections = -1;
};

struct ClientInfo {
    std::chrono::steady_clock::time_point lastActivityTime;
};

// Components
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
    int init() override;

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

    static bool isActive(Connection, int timeout);
    static bool validate(Connection);
};