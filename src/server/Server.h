//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "modules/ServerModuleManager.h"
#include "commands/CommandManager.h"
#include "ServerSignals.h"
#include <atomic>
#include <condition_variable>

// Forward declaration(s)
class Subsystem;
class Command;

class Server {
    Server();
    ~Server() = default;

    ServerModuleManager m_ModuleManager;
    CommandManager m_CommandManager;

    std::atomic<bool> m_Running;
    bool m_Daemonized;
    std::string m_WorkingDirectory;

    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;

public:
    // Singleton instance method
    static Server &instance();

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
    [[nodiscard]] ServerModuleManager &moduleManager() { return m_ModuleManager; }
    [[nodiscard]] CommandManager &commandManager() { return m_CommandManager; }

    // Daemon stuff
    void daemonize();

private:
    int init(int argc, char **argv);
};

#define server Server::instance()