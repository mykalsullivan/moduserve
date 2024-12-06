//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "Signal.h"
#include "ModuleManager.h"
#include "CommandManager.h"
#include <atomic>

class Server {
public signals:
    // Activated upon initialization
    static Signal<> initialized;

    // Activated upon server start
    static Signal<> started;

    // Activated upon server stop
    static Signal<> stopped;

    // Activated when a server module is added
    static Signal<> addedModule;

    // Activated when a server module stops
    static Signal<ServerModule> moduleStopped;

    // Activated when a server module crashes
    static Signal<ServerModule> moduleCrashed;

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

    // Run the server
    int run(int argc, char **argv);

    // Stops the server
    void stop();

    // Returns true if the server is running
    [[nodiscard]] bool isRunning() const { return m_Running; }

    // Returns true if the server is daemonized
    [[nodiscard]] bool isDaemonized() const { return m_Daemonized; }

    // Returns the current working directory
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

    // Adds a server command
    template<typename T, typename... Args>
    static void addCommand(Args &&... args);

    // Retrieves a server command by name
    template<typename T>
    static std::shared_ptr<T> getCommand();

    // Retrieve all the server commands tied to a server module
    template<typename T>
    static bool hasCommand();

private:
    // Daemonizes the server
    void daemonize();
};