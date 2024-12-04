//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "Signal.h"
#include <atomic>
#include <optional>

class Server {
public signals:
    static Signal<> finishedInitialization;
    static Signal<> serverRun;
    static Signal<> serverStop;
    static Signal<> serverDaemonized;
    static Signal<> serverAddModule;

private:
    std::atomic<bool> m_Running;
    bool m_Daemonized;

public:
    Server();
    virtual ~Server() = default;

    // Delete copy constructor and assignment operators
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    // Runtime stuff
    int run(int argc, char **argv);
    void stop();
    void daemonize();

    [[nodiscard]] bool isRunning() const { return m_Running; }
    [[nodiscard]] bool isDaemonized() const { return m_Daemonized; }
    [[nodiscard]] std::string workingDirectory() const;

    // Add a server module
    template<typename T, typename... Args>
    static void addModule(Args &&... args);

    // Retrieve a module by type
    template<typename T>
    static std::shared_ptr<T> getModule();

    // Retrieve a module by type (optional)
    template<typename T>
    static std::optional<std::shared_ptr<T>> getOptionalModule();

    // Returns true if the module is loaded
    template<typename T>
    static bool isModuleLoaded();
};