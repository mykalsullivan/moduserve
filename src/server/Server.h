//
// Created by msullivan on 11/8/24.
//

#pragma once
#include <atomic>
#include <condition_variable>

// Forward declarations
class ConnectionManager;
class UserManager;
class UserAuthenticator;
class MessageHandler;
class Connection;

class Server {
public:
    explicit Server(int port, int argc, char *argv[]);
    ~Server();

private:
    std::atomic<bool> m_Running;
    std::mutex m_Mutex;
    std::condition_variable m_CV;

    ConnectionManager *m_ConnectionManager;
    UserAuthenticator *m_UserAuthenticator;
    UserManager *m_UserManager;
    MessageHandler *m_MessageHandler;

public:
    int run();
    void stop();

    [[nodiscard]] ConnectionManager &connectionManager() const { return *m_ConnectionManager; }
    [[nodiscard]] UserAuthenticator &authenticatorService() const { return *m_UserAuthenticator; }
    [[nodiscard]] UserManager &userManager() const { return *m_UserManager; }
    [[nodiscard]] MessageHandler &messageHandler() const { return *m_MessageHandler; }

    [[nodiscard]] std::condition_variable &getCV() { return m_CV; }
    [[nodiscard]] bool isRunning() { return m_Running; }

private:

    // Command stuff
    void purgeClients();
    void announceMessage();
    void listClients();
    void displayServerInfo();
};