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
    Signal<void> onAdd;
    Signal<void> onRemove;

public:
    int init() override;
    [[nodiscard]] constexpr std::string name() const override { return "ConnectionSubsystem"; }

    [[nodiscard]] size_t size() const { return m_Connections.size(); }
    [[nodiscard]] bool empty() const { return m_Connections.empty(); }
    [[nodiscard]] std::unordered_map<int, Connection *>::iterator begin() { return m_Connections.begin(); }
    [[nodiscard]] std::unordered_map<int, Connection *>::iterator end() { return m_Connections.end(); }
    [[nodiscard]] int serverFD() const { return m_ServerFD; }

    bool add(Connection &connection);
    bool remove(int socketFD);
    [[nodiscard]] Connection *get(int fd);
    [[nodiscard]] Connection *operator[](int fd);

public:


private:
    void eventThreadWork();
    void acceptorThreadWork();

    std::vector<int> processConnections();
    void validateConnections();
    void purgeConnections(const std::vector<int> &connectionsToPurge);
    void processMessage(Connection &connection, const std::string &message);
};