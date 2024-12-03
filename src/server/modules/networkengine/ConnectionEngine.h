//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../ServerModule.h"
#include "Connection.h"
#include "server/ServerSignals.h"

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
    static void onStartup() {}
    static void onShutdown() {}

    static void beforeAccept(Connection) {}
    static void onAccept(Connection) {}
    static void afterAccept(Connection) {}

    static void beforeDisconnect(Connection) {}
    static void onDisconnect(Connection) {}
    static void afterDisconnect(Connection) {}

    static void beforeDataSend(Connection) {}
    static void onDataSend(Connection) {}
    static void afterDataSend(Connection) {}

    static void beforeDataReceive(Connection) {}
    static void onDataReceive(Connection) {}
    static void afterDataReceive(Connection) {}

    static void beforeDataBroadcast(Connection) {}
    static void onDataBroadcast(Connection) {}
    static void afterDataBroadcast(Connection) {}

    static void onReceiveKeepalive(Connection) {}

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