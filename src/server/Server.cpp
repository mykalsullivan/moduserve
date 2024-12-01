//
// Created by msullivan on 11/8/24.
//

#include "Server.h"
#include "ServerConnection.h"
#include "common/PCH.h"
#include <filesystem>

#include "subsystems/Subsystem.h"
#include "subsystems/connection_subsystem/ConnectionSubsystem.h"
#include "subsystems/message_subsystem/MessageSubsystem.h"
#include "subsystems/broadcast_subsystem/BroadcastSubsystem.h"
#include "subsystems/command_subsystem/CommandSubsystem.h"
#include "subsystems/user_subsystem/UserSubsystem.h"

void printUsage();
int port = 0;

Server::Server() : m_Running(false)
{}

Server &Server::instance()
{
    static Server instance;
    return instance;
}

int Server::run(int argc, char **argv)
{
    LOG(LogLevel::INFO, "Starting XServer...")

    int initResult = init(argc, argv);
    if (initResult != 0) return initResult;

    m_Running = true;

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

int Server::init(int argc, char **argv)
{
    // 1. Set working directory
    try
    {
        m_WorkingDirectory = std::filesystem::current_path().string();
        LOG(LogLevel::INFO, "Current working directory: " + m_WorkingDirectory);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        LOG(LogLevel::ERROR, "Failed to retrieve working directory");
        exit(EXIT_FAILURE);
    }

    // 2. Parse command-line arguments
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

    // 3. Create server connection
    auto serverConnection = new ServerConnection();

    // 4. Create socket
    if (!serverConnection->createSocket())
    {
        LOG(LogLevel::ERROR, "Failed to create socket");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    LOG(LogLevel::DEBUG, "Created server socket: " + std::to_string(serverConnection->getFD()));

    // 5. Create server address
    if (!serverConnection->createAddress(port))
    {
        LOG(LogLevel::ERROR, "Failed to create address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    LOG(LogLevel::DEBUG, "Successfully created server address (listening on all interfaces)");

    // 6. Bind socket to address
    if (!serverConnection->bindAddress())
    {
        LOG(LogLevel::ERROR, "Failed to bind address");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    LOG(LogLevel::DEBUG, "Successfully bound to address (" + serverConnection->getIP() + ':' + std::to_string(serverConnection->getPort()) + ')');

    // 7. Listen to incoming connections
    if (!serverConnection->startListening())
    {
        LOG(LogLevel::ERROR, "Cannot listen to incoming connections");
        delete serverConnection;
        exit(EXIT_FAILURE);
    }
    LOG(LogLevel::INFO, "Listening for new connections on " + serverConnection->getIP() + ':' +
                        std::to_string(serverConnection->getPort()) + ')');

    // 8. Register built-in subsystems
    registerSubsystem(std::make_unique<ConnectionSubsystem>(*serverConnection));
    registerSubsystem(std::make_unique<MessageSubsystem>());
    registerSubsystem(std::make_unique<BroadcastSubsystem>());
    registerSubsystem(std::make_unique<CommandSubsystem>());
    registerSubsystem(std::make_unique<UserSubsystem>());

    // 9. Initialize subsystems
    for (auto &ss : m_Subsystems)
    {
        int result = ss.second->init();
        if (result == 0)
            LOG(LogLevel::INFO, "Subsystem \"" + ss.second->name() + "\" initialized")
        else
        {
            LOG(LogLevel::ERROR, "Subsystem \"" + ss.second->name() + "\" failed to initialize")
            return 1;
        }
    }
    return 0;
}

void Server::registerSubsystem(std::unique_ptr<Subsystem> subservice)
{
    const std::string &serviceName = subservice->name();

    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    if (!m_Subsystems.contains(serviceName))
    {
        m_Subsystems[serviceName] = std::move(subservice);
        return;
    }
    throw std::runtime_error("Subsystem with name '" + serviceName + "' is already registered.");
}

Subsystem *Server::getSubsystem(const std::string &name) const
{
    // Ensure thread safety with a lock
    std::lock_guard lock(m_Mutex);

    auto it = m_Subsystems.find(name);
    if (it != m_Subsystems.end())
        return it->second.get();
    throw std::runtime_error("Subsystem with name \"" + name + "\" not found.");
}


void printUsage()
{
    std::cout << "Usage: program [-p port]" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}