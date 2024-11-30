//
// Created by msullivan on 11/8/24.
//

#include "Server.h"
#include "ServerConnection.h"
#include "ConnectionManager.h"
#include "MessageProcessor.h"
#include "BroadcastManager.h"
#include "CommandRegistry.h"
#include "UserManager.h"
#include "UserAuthenticator.h"
#include "../Logger.h"
#include <iostream>

int port = 0;

void printUsage()
{
    std::cout << "Usage: program [-p port]" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}

int Server::init(int argc, char **argv)
{
    // 1. Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1)
        switch (opt)
        {
            case 'p':
                port = std::stoi(optarg);
            break;
            case 'h':
                printUsage();
            return 0;
            case '?':
            default:
                printUsage();
            return -1;
        }

    // 2. Create server connection
    auto serverConnection = new ServerConnection();

    // 3. Create socket
    if (!serverConnection->createSocket()) {
        LOG(LogLevel::ERROR, "Failed to create socket");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //LOG(LogLevel::INFO, "Created server socket: " + std::to_string(serverConnection->getSocket()));

    // 4. Create server address
    if (!serverConnection->createAddress(port)) {
        LOG(LogLevel::ERROR, "Failed to create address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //LOG(LogLevel::INFO, "Successfully created server address (listening on all interfaces)");

    // 5. Bind socket to address
    if (!serverConnection->bindAddress()) {
        LOG(LogLevel::ERROR, "Failed to bind address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    //LOG(LogLevel::INFO, "Successfully bound to address (" + serverConnection->getIP() + ':' + std::to_string(serverConnection->getPort()) + ')');

    // 6. Listen to incoming connections
    if (!serverConnection->startListening()) {
        LOG(LogLevel::ERROR, "Cannot listen to incoming connections");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    LOG(LogLevel::INFO, "Listening for new connections on " + serverConnection->getIP() + ':' +
                        std::to_string(serverConnection->getPort()) + ')');

    // 7. Create sub-services
    m_ConnectionManager = std::make_unique<ConnectionManager>(*m_BroadcastManager, *m_MessageProcessor, *serverConnection, m_ServiceBarrier);
    m_MessageProcessor = std::make_unique<MessageProcessor>(*m_ConnectionManager, *m_BroadcastManager, *m_CommandRegistry, m_ServiceBarrier);
    m_BroadcastManager = std::make_unique<BroadcastManager>(*m_ConnectionManager, *m_MessageProcessor, m_ServiceBarrier);
    m_CommandRegistry = std::make_unique<CommandRegistry>(m_ServiceBarrier);
    m_UserManager = std::make_unique<UserManager>(*m_ConnectionManager, *m_UserAuthenticator, m_ServiceBarrier);
    m_UserAuthenticator = std::make_unique<UserAuthenticator>(m_ServiceBarrier);
    LOG(LogLevel::INFO, "All sub-services initialized");

    return 0;
}

Server::Server() : m_Running(false), m_ServiceBarrier(6)
{}

Server &Server::instance()
{
    static Server instance;
    return instance;
}

int Server::run(int argc, char **argv)
{
    LOG(LogLevel::INFO, "Starting ChatApplicationServer...")
    
    int initResult = init(argc, argv);
    if (initResult != 0) return initResult;

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
