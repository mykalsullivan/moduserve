//
// Created by msullivan on 11/8/24.
//

#pragma once
#include <memory>
#include <atomic>
#include <condition_variable>
#include <unordered_map>

// Forward declaration(s)
class Subsystem;
class Command;

class Server {
    Server();
    ~Server() = default;

public:
    // Singleton instance method
    static Server &instance();

    // Delete copy constructor and assignment operators
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

private:
    std::atomic<bool> m_Running;
    bool m_Daemonized;
    std::string m_WorkingDirectory;
    std::unordered_map<std::string, std::unique_ptr<Subsystem>> m_Subsystems;

    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;

public:
    // Runtime stuff
    int run(int argc, char **argv);
    void stop();

    [[nodiscard]] bool isRunning() const { return m_Running; }
    [[nodiscard]] bool isDaemonized() const { return m_Daemonized; }
    [[nodiscard]] std::string workingDirectory() const { return m_WorkingDirectory; }

    // Subservice stuff
    void registerSubsystem(std::unique_ptr<Subsystem> subservice);
    Subsystem *subsystem(const std::string &name) const;

    // Command stuff
    void registerCommand(std::unique_ptr<Command> command);
    Command *command(const std::string &name) const;

    // Daemon stuff
    void daemonize();

private:
    int init(int argc, char **argv);
};

#define server Server::instance()