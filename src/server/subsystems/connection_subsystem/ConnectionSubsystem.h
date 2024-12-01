//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Subsystem.h"
#include "../Signal.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <barrier>

// Forward declaration(s)
class Connection;
class ServerConnection;

class ConnectionSubsystem : public Subsystem {
public:
    explicit ConnectionSubsystem(ServerConnection &serverConnection);
    ~ConnectionSubsystem() override;

private:
    std::unordered_map<int, Connection *> m_Connections;
    int m_ServerFD;

    std::thread m_AcceptorThread;
    std::thread m_EventThread;
    std::barrier<> m_ThreadBarrier;

    mutable std::mutex m_Mutex;
    std::condition_variable m_EventCV;

public signals:
    Signal<void()> onAccept;
    Signal<void()> onDisconnect;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "ConnectionSubsystem"; }

    [[nodiscard]] size_t size() const { return m_Connections.size(); }
    [[nodiscard]] bool empty() const { return m_Connections.empty(); }
    [[nodiscard]] auto begin() { return m_Connections.begin(); }
    [[nodiscard]] auto end() { return m_Connections.end(); }
    [[nodiscard]] int serverFD() const { return m_ServerFD; }

    bool add(Connection &connection);
    bool remove(int socketFD);
    [[nodiscard]] Connection *get(int fd);
    [[nodiscard]] Connection *operator[](int fd);

private:
    void eventThreadWork();
    void acceptorThreadWork();

    void processConnectionsInternal(const std::function<bool(Connection *)>& connectionPredicate);
    void processConnections();
    void validateConnections();

    void processMessage(Connection &connection, const std::string &message);
};