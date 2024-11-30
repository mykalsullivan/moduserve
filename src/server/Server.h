//
// Created by msullivan on 11/8/24.
//

#pragma once
#include <memory>
#include <atomic>
#include <condition_variable>
#include <barrier>

// Forward declaration(s)
class ConnectionManager;
class MessageProcessor;
class BroadcastManager;
class CommandRegistry;
class UserManager;
class UserAuthenticator;

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
    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;
    std::barrier<> m_ServiceBarrier;

    std::unique_ptr<ConnectionManager> m_ConnectionManager {};
    std::unique_ptr<MessageProcessor> m_MessageProcessor {};
    std::unique_ptr<BroadcastManager> m_BroadcastManager {};
    std::unique_ptr<CommandRegistry> m_CommandRegistry {};
    std::unique_ptr<UserManager> m_UserManager {};
    std::unique_ptr<UserAuthenticator> m_UserAuthenticator {};

public:
    int run(int argc, char **argv);
    void stop();
    [[nodiscard]] bool isRunning() { return m_Running; }

private:
    int init(int argc, char **argv);
};