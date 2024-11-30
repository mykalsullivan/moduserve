//
// Created by msullivan on 11/8/24.
//

#include "Client.h"

Client::Client(int argc, char *argv[]) : m_Running(true)
{
    handleUserInput();
}

Client::~Client()
{
    // Delete the connection to server (if any)
    if (!m_Connection)
        delete m_Connection;
}

bool Client::connectToServer(const std::string &ip, int port, int timeout)
{
    // 1. Create connection
    m_Connection = new ClientConnection();

    // 1.1 Create socket
    if (!m_Connection->createSocket() || !m_Connection->createAddress(ip, port)) {
        std::cerr << "Failed to create socket or address\n";
        delete m_Connection;
        m_Connection = nullptr;
        return false;
    }

    // 4. Attempt to connect to server
    if (m_Connection->connectToServer() < 0) {
        std::cerr << "Failed to connect to server (" << m_Connection->getIP() << ':' << m_Connection->getPort() << ")\n";
        delete m_Connection;
        m_Connection = nullptr;
        return false;
    }

    // Start message listening thread
    m_Connection->startMessagePollingThread();

    // Start keepalive thread
    m_Connection->startKeepaliveThread();

    std::cout << "\nSuccessfully connected to server (" << m_Connection->getIP() << ':' << m_Connection->getPort() << ")\n";
    return true;

    // Start authentication
    std::cout << "Starting authentication process...\n";
    if (!authenticate()) {
        std::cout << "Authentication failed; disconnecting...\n";
        delete m_Connection;
        m_Connection = nullptr;
        return false;
    }
    std::cout << "Successfully authenticated with server\n";

    // Start message listening thread
    //startMessagePollingThread();

    // Start keepalive thread
    //startKeepaliveThread();
    return true;
}

bool Client::authenticate()
{
    // 1. Receive authentication prompt from the server
    std::string prompt = receiveMessage();
    if (prompt.empty()) {
        std::cerr << "Failed to receive server prompt\n";
        return false;
    }
    std::cout << prompt << '\n';

    // 2. Prompt user to provide username
    std::string username;
    std::cout << "Username: ";
    std::getline(std::cin, username);

    // 3. Send username to the server
    sendMessage(username);

    // 4. Receive the server's response
    std::string serverResponse = receiveMessage();
    if (serverResponse.empty()) {
        std::cerr << "Failed to receive server response\n";
        return false;
    }

    std::cout << serverResponse << '\n';

    // 5. Check if authentication was successful
    if (serverResponse.find("Authentication successful") != std::string::npos) {
        // Successful authentication
        m_Username = username;

        // Confirm that you acknowledge the server's authentication
        sendMessage("OK");
        return true;
    } else {
        // Authentication failed
        return false;
    }
}

bool Client::sendMessage(const std::string &message) const {
    if (m_Connection != nullptr)
        return m_Connection->sendData(message);
    return false;
}

std::string Client::receiveMessage() const {
    if (m_Connection != nullptr)
        return m_Connection->receiveMessage();
    return "";
}

void Client::handleUserInput()
{
    std::cout << "Socket Practice Client by M. ---\n";

    std::string input;
    while (m_Running) {
        std::getline(std::cin, input);

        if (input.empty()) {
            // Do nothing.
        }
        else if (input == "/help") {
            std::stringstream ss;
            ss << "\nHelp Menu:\n";
            ss << "/info                : Displays the current server connection info\n";
            ss << "/connect <ip> <port> : Connect to a different server\n";
            ss << "/disconnect          : Disconnects from the connected server\n";
            ss << "/quit                : Disconnect from the server\n";
            ss << "/stop                : Stop the server\n";
            std::cout << ss.str() << '\n';
        }
        else if (input == "/info") {
            if (m_Connection != nullptr) {
                std::cout << "Connected to " << m_Connection->getIP() << ':' << m_Connection->getPort() << '\n';
            } else {
                std::cout << "There is no current server connection\n";
            }
        }
        else if (input.substr(0, 8) == "/connect") {
            std::istringstream iss(input);
            std::string command;
            std::string newIP;
            int newPort;

            iss >> command >> newIP >> newPort;

            if (newIP.empty() || newPort <= 0) {
                std::cout << "Invalid IP or port for connection. Usage: /connect <ip> <port>\n";
                continue;
            }
            // Close current connection
            if (m_Connection != nullptr)
                m_Connection->closeConnection();
            std::cout << "Connecting to server " << newIP << ":" << newPort << "...\n";
            connectToServer(newIP, newPort, 5);
        }
        else if (input == "/disconnect") {
            if (m_Connection != nullptr)
                m_Connection->closeConnection();
        }
        else if (input == "/quit") {
            std::cout << "Quitting...\n";
            if (m_Connection != nullptr)
                m_Connection->closeConnection();
            m_Running = false;
        }
        else if (input == "/stop") {
            std::cout << "Telling the server to stop and disconnecting...\n";
            sendMessage(input);
            if (m_Connection != nullptr)
                m_Connection->closeConnection();
        }
        else if (!input.empty()) {
            sendMessage(input);  // Send the message to the server
        }
    }
}
