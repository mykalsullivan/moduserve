//
// Created by msullivan on 11/8/24.
//

#include "Server.h"
#include "common/PCH.h"
#include <filesystem>

#include "subsystems/connection_subsystem/ConnectionSubsystem.h"
#include "subsystems/message_subsystem/MessageSubsystem.h"

void printUsage();

Server::Server() : m_Running(false), m_Daemonized(false)
{}

Server &Server::instance()
{
    static Server instance;
    return instance;
}

int Server::run(int argc, char **argv)
{
    logMessage(LogLevel::INFO, "Starting XServer...");

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
    logMessage(LogLevel::INFO, "Server shutting down...");
}

int Server::init(int argc, char **argv)
{
    // 1. Set working directory
    try
    {
        m_WorkingDirectory = std::filesystem::current_path().string();
        logMessage(LogLevel::INFO, "Current working directory: " + m_WorkingDirectory);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        logMessage(LogLevel::ERROR, "Failed to retrieve working directory");
        exit(EXIT_FAILURE);
    }

    // 2. Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1)
        switch (opt)
        {
            case 'p':
                //port = std::stoi(optarg);
            break;
            case 'h':
                printUsage();
            return 0;
            case '?':
            default:
                printUsage();
            return -1;
        }

    // 8. Register built-in subsystems
    m_SubsystemManager.registerSubsystem(std::make_unique<ConnectionSubsystem>());
    m_SubsystemManager.registerSubsystem(std::make_unique<MessageSubsystem>());

    // 9. Initialize subsystems
    for (auto &ss : m_SubsystemManager)
    {
        int result = ss.second->init();
        if (result == 0)
            logMessage(LogLevel::INFO, "Subsystem \"" + ss.second->name() + "\" initialized");
        else
        {
            logMessage(LogLevel::ERROR, "Subsystem \"" + ss.second->name() + "\" failed to initialize");
            return 1;
        }
    }
    return 0;
}

void printUsage()
{
    std::cout << "Usage: program [-p port]" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}