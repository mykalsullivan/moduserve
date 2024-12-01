//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Subsystem.h"
#include "server/Signal.h"
#include <barrier>

// Forward declaration(s)
class Connection;

class ConnectionSubsystem : public Subsystem {
    std::thread m_AcceptorThread;
    std::thread m_EventThread;
    mutable std::mutex m_ThreadMutex;
    std::condition_variable m_ThreadCV;
    std::barrier<> m_ThreadBarrier;

public:
    ConnectionSubsystem();
    ~ConnectionSubsystem() override;

public signals:
    Signal<const Connection &> onConnect;
    Signal<const Connection &> onDisconnect;
    Signal<const Connection &, const std::string &> onBroadcast;

public slots:
    static void onConnectFunction(const Connection &connection);
    static void onDisconnectFunction(const Connection &connection);
    static void broadcastMessage(const Connection &sender, const std::string &message);

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "ConnectionSubsystem"; }

private:
    void processConnectionsInternal(const std::function<bool(Connection *)>& connectionPredicate);
    void processConnections();
    void validateConnections();
};