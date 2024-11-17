//
// Created by msullivan on 11/8/24.
//

#pragma once
#include <atomic>

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

    ConnectionManager *m_ConnectionManager;
    UserAuthenticator *m_UserAuthenticator;
    UserManager *m_UserManager;
    MessageHandler *m_MessageHandler;

public:
    void stop();

    [[nodiscard]] ConnectionManager &connectionManager() const { return *m_ConnectionManager; }
    [[nodiscard]] UserAuthenticator &authenticatorService() const { return *m_UserAuthenticator; }
    [[nodiscard]] UserManager &userManager() const { return *m_UserManager; }
    [[nodiscard]] MessageHandler &messageHandler() const { return *m_MessageHandler; }

private:
    // Client management
    void acceptClient();
    bool authenticateClient(Connection &client);

    // User input
    void handleUserInput();

    // Command stuff
    void purgeClients();
    void announceMessage();
    void listClients();
    void displayServerInfo();
};