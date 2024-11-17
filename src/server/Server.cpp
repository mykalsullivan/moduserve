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

Server::Server(int port, int argc, char *arv[]) : m_Running(false), m_ConnectionManager(nullptr), m_UserManager(nullptr), m_MessageHandler(nullptr)
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
    m_UserManager = new UserManager(*this);
    m_MessageHandler = new MessageHandler(*this);

    m_Running = true;
    handleUserInput();
}

Server::~Server()
{
    // Stop all sub-processes
    m_ConnectionManager->stop();
    m_MessageHandler->stop();

    // Delete sub-processes
    delete m_ConnectionManager;
    delete m_MessageHandler;
    delete m_UserAuthenticator;
    delete m_UserManager;

    // Gracefully shutdown server socket (if necessary)
    shutdown(m_ConnectionManager->getServerSocketID(), SHUT_RDWR);
}

void Server::handleUserInput()
{
    std::string input;
    while (m_Running)
    {
        std::getline(std::cin, input);
        if (!m_Running) break;  // Early exit if server is stopping

        if (input.empty()) {
            // Do nothing
        }
        else if (input == "/stop") {
            stop();
        }
        else if (input == "/list") {
            listClients();
        }
        else if (input == "/help") {
            std::cout << "Available commands:\n";
            std::cout << "/announce - Send a message to all clients\n";
            std::cout << "/list - List all connected clients\n";
            std::cout << "/info - Display server info (IP, port, number of users)\n";
            std::cout << "/help - Display this help message\n";
            std::cout << "/stop - Stop the server\n";
        }
        else if (input == "/announce") {
            announceMessage();
        }
        else if (input == "/info") {
            displayServerInfo();
        }
        else {
            std::cout << "Unknown input: " << input << "\n";
        }
    }
}

void Server::stop() {
    m_Running = false;
    Logger::instance().logMessage(LogLevel::INFO, "Server shutting down...");
}

void Server::acceptClient()
{
    auto client = new Connection();
    auto clientAddress = client->getAddress();
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientFD = accept(m_ConnectionManager->getServerSocketID(), reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
    if (clientFD < 0) {
        Logger::instance().logMessage(LogLevel::ERROR, "Client attempted to connect, but failed to get FD");
        return;
    }

    client->setSocket(clientFD);
    client->setAddress(clientAddress);

    // Add connection to the connection list
    if (!m_ConnectionManager->addConnection(client)) {
        Logger::instance().logMessage(LogLevel::ERROR, "Connection already exists");
        delete client;
    }
    Logger::instance().logMessage(LogLevel::DEBUG, "Client (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") attempting to connect");

    // // Create a user for the client. If the client fails authentication, it will delete this user
    // auto newUser = new User(clientFD, *client);
    // m_UserManager->addUser(clientFD, newUser);

    // Start the validation process
    Logger::instance().logMessage(LogLevel::DEBUG, "Started validation process for client (" + client->getIP() + ':' + std::to_string(client->getPort()) + ")...");
    if (!authenticateClient(*client)) {
        m_ConnectionManager->removeConnection(clientFD); // If authentication fails, disconnect the client
        Logger::instance().logMessage(LogLevel::DEBUG, "Client (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") failed to authenticate");
        return;
    }

    // Further validation can be added here (e.g., checking if the username is taken)

    // Announce that the client has joined to all users
    Logger::instance().logMessage(LogLevel::DEBUG, "Client (" + client->getIP() + ':' + std::to_string(client->getPort()) + ") successfully authenticated");
    m_MessageHandler->broadcastMessage(client, client->getIP() + ':' + std::to_string(client->getPort()) + " connected... welcome!");
}

bool Server::authenticateClient(Connection &client)
{
    // Prompt client for username
    const std::string welcomeMessage = "\"Welcome! Please provide your username.\"";
    client.sendMessage(welcomeMessage);

    // Set up a pollfd for the client socket
    pollfd clientPollFD = { client.getSocket(), POLLIN, 0 };

    // Set timeout time in seconds
    int timeout = 30;

    // Poll the client
    int result = poll(&clientPollFD, 1, timeout * 1000);
    if (result == -1) {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to poll client socket during authentication");
        return false;
    }

    // Timeout occurred
    if (result == 0) {
        std::string errorMessage = "Authentication timeout... please try again later";
        client.sendMessage(errorMessage);
        Logger::instance().logMessage(LogLevel::INFO, "Client (" + client.getIP() + ':' + std::to_string(client.getPort()) + ") timed out");
        return false;
    }

    // If the poll returns with data ready to read, we can now receive the username
    if (clientPollFD.revents & POLLIN) {
        std::string username = client.receiveMessage();

        // Usernames must not be empty
        if (username.empty()) {
            std::string errorMessage = "Username is required; disconnecting...";
            client.sendMessage(errorMessage);
            std::cerr << "Client (" << client.getIP() << ':' << client.getPort() << ") sent an empty username\n";
            return false;
        }
        std::cout << "Client sent username \'" << username << "\'\n";
        //m_UserManager[client.getSocket()]->setUsername(username);

        // Let the client know that they authenticated successfully
        std::cout << "Letting the client know it successfully authenticated...\n";
        client.sendMessage("Authentication successful");

        // Check if the client understands that it has successfully authenticated
        std::string confirmation = client.receiveMessage();
        if (confirmation.find("OK"))
            return true;
        std::cerr << "Client failed to acknowledge successful authentication\n";
        return false;
    }
    return true;
}

void Server::announceMessage()
{
    std::cout << "Enter the announcement message: ";
    std::string message;
    std::getline(std::cin, message);

    // Broadcast the message to all clients
    m_MessageHandler->broadcastMessage(m_ConnectionManager->getConnection(m_ConnectionManager->getServerSocketID()), message);
    Logger::instance().logMessage(LogLevel::INFO,"Announcement sent to all clients.");
}

void Server::purgeClients()
{
    std::cout << "Purging all clients...\n";

    // Create a copy of the client list to avoid modifying the container while iterating
    std::vector<int> clientsToPurge;
    for (auto connection : *m_ConnectionManager) {
        clientsToPurge.push_back(connection.first);  // Add client FD to the list
    }

    // Iterate over the copy of client IDs and kick each one
    for (int clientFD : clientsToPurge)
        m_ConnectionManager->removeConnection(clientFD);  // Remove (kick) the client

    std::cout << "All clients have been purged.\n";
}

void Server::listClients() {
    if (m_ConnectionManager->empty()) {
        std::cout << "No clients connected.\n";
    } else {
        std::cout << "Connected clients:\n";

        for (auto client : *m_ConnectionManager)
        {
            // std::cout << "Client \'" << &m_UserManager[client.first] << '\''
            //           << " (" << client.second->getIP() << ':' << std::to_string(client.second->getPort()) << ")\n";
            std::cout << "Client " << client.first << ": " << client.second->getIP() << ':' << std::to_string(client.second->getPort()) << ")\n";
        }
    }
}

void Server::displayServerInfo()
{
    // Get the server's IP address (use local address for simplicity)
    sockaddr_in localAddress {};
    socklen_t len = sizeof(localAddress);
    if (getsockname(m_ConnectionManager->getServerSocketID(), reinterpret_cast<sockaddr*>(&localAddress), &len) == -1) {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to get local address");
        return;
    }

    std::string serverIP = inet_ntoa(localAddress.sin_addr);
    int serverPort = ntohs(localAddress.sin_port);

    // Count the number of connected users
    size_t numUsers = m_ConnectionManager->size();

    std::cout << "Server Info:\n";
    std::cout << "IP Address: " << serverIP << "\n";
    std::cout << "Port: " << serverPort << "\n";
    std::cout << "Connected Users: " << numUsers << "\n";
}
