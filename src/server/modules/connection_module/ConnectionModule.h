//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../ServerModule.h"
#include "server/Signal.h"
#include <condition_variable>
#include <barrier>

// Forward declaration(s)
class OldConnection;

class ConnectionModule : public ServerModule {
    std::thread m_AcceptorThread;
    std::thread m_EventThread;
    mutable std::mutex m_ThreadMutex;
    std::condition_variable m_ThreadCV;
    std::barrier<> m_ThreadBarrier;

    static int m_ServerFD;

public:
    ConnectionModule();
    ~ConnectionModule() override;

public signals:
    SIGNAL(onConnect, const OldConnection &);
    SIGNAL(onDisconnect, const OldConnection &);
    SIGNAL(onBroadcast, const OldConnection &, const std::string &);

private slots:
    SLOT(onConnectFunction, void, const OldConnection &OldConnection);
    SLOT(onDisconnectFunction, void, const OldConnection &OldConnection);
    SLOT(broadcastMessage, void, const OldConnection &sender, const std::string &data);

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "OldConnectionSubsystem"; }

private:
    void processConnectionsInternal(const std::function<bool(OldConnection *)>& OldConnectionPredicate);
    void processConnections();
    void validateConnections();
};