//
// Created by msullivan on 11/8/24.
//

#include "Server.h"
#include "ServerConnection.h"
#include "ConnectionManager.h"
#include "UserAuthenticator.h"
#include "UserManager.h"
#include "MessageHandler.h"
#include "Logger.h"
#include <iostream>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <arpa/inet.h>

Server::Server(int port, int argc, char *arv[]) : m_Running(true), m_ConnectionManager(nullptr), m_UserManager(nullptr), m_MessageHandler(nullptr)
{
    // 1. Create server connection
    auto serverConnection = new ServerConnection();

    // 2. Create socket
    if (!serverConnection->createSocket())
    {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to create socket");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //Logger::instance().logMessage(LogLevel::INFO, "Created server socket: " + std::to_string(serverConnection->getSocket()));

    // 2. Create server address
    if (!serverConnection->createAddress(port)) {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to create address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //Logger::instance().logMessage(LogLevel::INFO, "Successfully created server address (listening on all interfaces)");

    // 3. Bind socket to address
    if (!serverConnection->bindAddress())
    {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to bind address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //Logger::instance().logMessage(LogLevel::INFO, "Successfully bound to address (" + serverConnection->getIP() + ':' + std::to_string(serverConnection->getPort()) + ')');

    // 5. Listen to incoming connections
    if (!serverConnection->startListening()) {
        Logger::instance().logMessage(LogLevel::ERROR, "Cannot listen to incoming connections");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    Logger::instance().logMessage(LogLevel::INFO, "Listening for new connections on " + serverConnection->getIP() + ':' + std::to_string(serverConnection->getPort()) + ')');

    // Create sub-processes
    m_ConnectionManager = new ConnectionManager(*this, serverConnection);
    m_MessageHandler = new MessageHandler(*this);
    m_UserManager = new UserManager(*this);
    m_UserAuthenticator = new UserAuthenticator(*this);
}

Server::~Server()
{
    // Gracefully shutdown server socket (if necessary)
    shutdown(m_ConnectionManager->getServerSocketID(), SHUT_RDWR);

    // Delete sub-processes
    delete m_ConnectionManager;
    delete m_MessageHandler;
    delete m_UserManager;
    delete m_UserAuthenticator;
}

int Server::run()
{
    std::unique_lock lock(m_Mutex);
    m_CV.wait(lock, [this] {
        return !m_Running;
    });
    return 0;
}

// This will need to notify all the sub-processes to stop because they will be detached from the main server thread
void Server::stop()
{
    m_Running = false;
    m_CV.notify_one();
    Logger::instance().logMessage(LogLevel::INFO, "Server shutting down...");
}
