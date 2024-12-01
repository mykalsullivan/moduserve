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
    SIGNAL(onConnect, const Connection &);
    SIGNAL(onDisconnect, const Connection &);
    SIGNAL(onBroadcast, const Connection &, const std::string &);

private slots:
    SLOT(onConnectFunction, void, const Connection &connection);
    SLOT(onDisconnectFunction, void, const Connection &connection);
    SLOT(broadcastMessage, void, const Connection &sender, const std::string &message);

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "ConnectionSubsystem"; }

private:
    void processConnectionsInternal(const std::function<bool(Connection *)>& connectionPredicate);
    void processConnections();
    void validateConnections();
};