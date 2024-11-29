//
// Created by msullivan on 11/8/24.
//

#include "Server.h"
#include "ServerConnection.h"
#include "ConnectionManager.h"
#include "UserAuthenticator.h"
#include "UserManager.h"
#include "../Logger.h"
#include <iostream>

int port = 0;

void printUsage()
{
    std::cout << "Usage: program [-p port]" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}

int processArgs(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = std::stoi(optarg);
            break;
            case 'h':
                printUsage();
            break;
            case '?':
            default:
                printUsage();
            return -1;
        }
    }
    return 0;
}

ServerConnection *init(int argc, char **argv)
{
    LOG(LogLevel::INFO, "Starting ChatApplicationServer...")
    if (processArgs(argc, argv) < 0)
        exit(EXIT_FAILURE);

    // 1. Create server connection
    auto serverConnection = new ServerConnection();

    // 2. Create socket
    if (!serverConnection->createSocket()) {
        LOG(LogLevel::ERROR, "Failed to create socket");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //LOG(LogLevel::INFO, "Created server socket: " + std::to_string(serverConnection->getSocket()));

    // 2. Create server address
    if (!serverConnection->createAddress(port)) {
        LOG(LogLevel::ERROR, "Failed to create address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //LOG(LogLevel::INFO, "Successfully created server address (listening on all interfaces)");

    // 3. Bind socket to address
    if (!serverConnection->bindAddress()) {
        LOG(LogLevel::ERROR, "Failed to bind address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //LOG(LogLevel::INFO, "Successfully bound to address (" + serverConnection->getIP() + ':' + std::to_string(serverConnection->getPort()) + ')');

    // 5. Listen to incoming connections
    if (!serverConnection->startListening()) {
        LOG(LogLevel::ERROR, "Cannot listen to incoming connections");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    LOG(LogLevel::INFO, "Listening for new connections on " + serverConnection->getIP() + ':' +
                        std::to_string(serverConnection->getPort()) + ')');
    return serverConnection;
}

Server::Server(int argc, char **argv) : m_Running(true),
                                        m_ConnectionManager(*this, *init(argc, argv)),
                                        m_MessageHandler(*this),
                                        m_BroadcastManager(*this),
                                        m_UserManager(*this),
                                        m_UserAuthenticator(*this)
{}

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
    LOG(LogLevel::INFO, "Server shutting down...");
}
