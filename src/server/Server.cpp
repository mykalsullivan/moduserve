//
// Created by msullivan on 11/8/24.
//

#include "Server.h"
#include "common/PCH.h"
#include "ModuleManager.h"
#include "modules/NetworkEngine.h"
#include "modules/MessageProcessor.h"
#include "modules/Logger.h"
#include <getopt.h>
#include <filesystem>

// Forward declaration(s)
void printUsage();

// Static variables
std::string g_WorkingDirectory;
std::mutex g_ServerMutex;
std::condition_variable g_ServerCV;

int init(int, char **);

Server::Server() : m_Running(false), m_Daemonized(false)
{}

int Server::run(int argc, char **argv)
{
    int initResult = init(argc, argv);
    if (initResult != 0) return initResult;

    m_Running = true;

    std::unique_lock lock(g_ServerMutex);
    g_ServerCV.wait(lock, [this] {
        return !m_Running;
    });
    return 0;
}

// This will need to notify all the sub-processes to stop because they will be detached from the main server thread
void Server::stop()
{
    m_Running = false;
    g_ServerCV.notify_one();
    Logger::log(LogLevel::Info, "Server shutting down...");
}

void Server::daemonize()
{
    // Do nothing for now
}

template<typename T>
std::shared_ptr<T> Server::getModule()
{
    return ModuleManager::instance().getModule<T>();
}

template<typename T, typename... Args>
void Server::addModule(Args &&... args)
{
    ModuleManager::registerModule<T>(std::forward<Args>(args)...);
}

template<typename T>
std::optional<std::shared_ptr<T>> Server::getOptionalModule()
{
    return ModuleManager::instance().getOptionalModule<T>();
}

template<typename T>
bool Server::isModuleLoaded()
{
    auto module = ModuleManager::instance().getModule<T>();
    return module ? true : false;
}

int init(int argc, char **argv)
{
    Logger::log(LogLevel::Info, "Starting XServer...");

    // 1. Set working directory
    try
    {
        g_WorkingDirectory = std::filesystem::current_path().string();
        Logger::log(LogLevel::Info, "Current working directory: " + g_WorkingDirectory);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        Logger::log(LogLevel::Error, "Failed to retrieve working directory");
        return 0;
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

    // 8. Add and initialize built-in modules
    ModuleManager::instance().registerModule<Logger>();
    ModuleManager::instance().registerModule<NetworkEngine>();
    ModuleManager::instance().registerModule<MessageProcessor>();
    ModuleManager::instance().initializeModules();
    ModuleManager::instance().startModules();
    return 0;
}

void printUsage()
{
    std::cout << "Usage: program [-p port]" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}