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
    std::string m_WorkingDirectory;
    std::unordered_map<std::string, std::unique_ptr<Subsystem>> m_Subservices;

    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;

public:
    // Runtime stuff
    int run(int argc, char **argv);
    void stop();
    [[nodiscard]] bool isRunning() { return m_Running; }
    [[nodiscard]] std::string workingDirectory() const { return m_WorkingDirectory; }

    // Subservice stuff
    void registerSubsystem(std::unique_ptr<Subsystem> subservice);
    Subsystem *subsystem(const std::string &name) const;

private:
    int init(int argc, char **argv);
};