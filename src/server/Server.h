//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "ServerSignal.h"
#include <atomic>
#include <condition_variable>

// Forward declaration(s)
class Subsystem;
class Command;

class Server {
public signals:
    Signal<> finishedInitialization;

public slots:


private:
    Server();

    std::atomic<bool> m_Running;
    bool m_Daemonized;
    std::string m_WorkingDirectory;

public:
    // Singleton instance method
    static Server &instance();
    ~Server() = default;

    // Delete copy constructor and assignment operators
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    // Runtime stuff
    int run(int argc, char **argv);
    void stop();

    [[nodiscard]] bool isRunning() const { return m_Running; }
    [[nodiscard]] bool isDaemonized() const { return m_Daemonized; }
    [[nodiscard]] std::string workingDirectory() const { return m_WorkingDirectory; }

    // Daemon stuff
    void daemonize();

private:
    int init(int argc, char **argv);
};

#define server Server::instance()