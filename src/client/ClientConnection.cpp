//
// Created by msullivan on 11/10/24.
//

#include "ClientConnection.h"

#include <cstring>

#include "../Logger.h"
#include <iostream>
#include <arpa/inet.h>

ClientConnection::ClientConnection()
{}

ClientConnection::~ClientConnection()
{
    stopKeepaliveThread();
    stopMessagePollingThread();
}

bool ClientConnection::createAddress(const std::string& ip, int port)
{
    m_Address.sin_family = AF_INET;         // Use IPv4
    m_Address.sin_port = htons(port);       // Bind port

    // Convert IP address from string to socket address
    if (inet_pton(AF_INET, ip.c_str(), &m_Address.sin_addr) <= 0) {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to convert IP address from string");
        return false;
    }
    return true;
}

int ClientConnection::connectToServer()
{
    Logger::instance().logMessage(LogLevel::DEBUG, "Attempting to connect to server at " + getIP() + ':' + std::to_string(getPort()));

    // Attempt to connect
    int connectionResult = connect(m_SocketFD, reinterpret_cast<sockaddr *>(&m_Address), sizeof(m_Address));

    // Immediately return if successful
    if (connectionResult == 0) {
        Logger::instance().logMessage(LogLevel::DEBUG, "Connection successful");
        return 1;
    }

    // Handle errors
    if (connectionResult == -1) {
        // Handle errors or retry based on the errno
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No pending connections, can retry after a short delay or poll
            std::cout << "No incoming connections right now\n";
            Logger::instance().logMessage(LogLevel::ERROR, "No incoming connections right now");
            return -1;
        }
        else if (errno == EINTR) {
            // Interrupted by signal, retry
            Logger::instance().logMessage(LogLevel::ERROR, "Interrupted by a signal");
            return -1;
        }
        else if (errno == ECONNREFUSED) {
            Logger::instance().logMessage(LogLevel::ERROR, "Connection refused");
            return -1;
        }
        else if (errno == ECONNABORTED) {
            Logger::instance().logMessage(LogLevel::ERROR, "Connection aborted");
            return -1;
        }
    }

    // If connection is in progress, use poll to wait for it
    if (connectionResult == -1 && errno == EINPROGRESS) {
        pollfd pfd {};
        pfd.fd = m_SocketFD;
        pfd.events = POLLOUT; // We are waiting for the socket to become writable (i.e., connected)
        pfd.revents = 0;

        int result = poll(&pfd, 1, 15000); // 30-second timeout
        Logger::instance().logMessage(LogLevel::DEBUG, "Poll result: " + std::to_string(result));

        if (result <= 0) {
            Logger::instance().logMessage(LogLevel::ERROR, "Connection timed out or poll failed");
            return 0;
        }

        // If poll() succeeds, check if the connection was successful
        if (pfd.revents & POLLOUT) {
            // Check if the socket is writable (connection is completed)
            int socketError = 0;
            socklen_t len = sizeof(socketError);
            if (getsockopt(m_SocketFD, SOL_SOCKET, SO_ERROR, &socketError, &len) == -1) {
                Logger::instance().logMessage(LogLevel::ERROR, "Failed to check socket error after poll");
                return -2;
            }

            // Connection failed for another reason
            if (socketError != 0) {
                Logger::instance().logMessage(LogLevel::ERROR, "Connection failed: " + socketError);

                return -3;
            }
            return 1;
        }
    }
    Logger::instance().logMessage(LogLevel::ERROR, "Failed to connect to server: error " + std::to_string(errno));
    return -1;
}

void ClientConnection::closeConnection()
{
    if (m_SocketFD != -1) {
        Logger::instance().logMessage(LogLevel::INFO, "Closing connection to " + getIP() + ':' + std::to_string(getPort()));

        close(m_SocketFD);
        m_SocketFD = -1;
    }
    stopMessagePollingThread();
    stopKeepaliveThread();
}


std::string ClientConnection::receiveMessage() const
{
    char buffer[1024];
    ssize_t bytesReceived = recv(m_SocketFD, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        return std::string(buffer, bytesReceived);
    } else if (bytesReceived == 0) {
        std::cerr << "Connection closed by peer\n";
        return "";
    } else {
        std::cerr << "Error receiving message\n";
        return "";
    }
}

void ClientConnection::startMessagePollingThread() {
    m_MessagePolling = true;
    m_MessagePollingThread = std::thread([this] {
        // Initialize pollfd
        pollfd pfd {};

        while (m_MessagePolling) {
            //Logger::instance().logMessage(LogLevel::DEBUG, "Polling...");
            pfd.fd = m_SocketFD;       // Set the file descriptor
            pfd.events = POLLIN;       // Make it poll events
            pfd.revents = 0;           // Initialize the events to 0

            int result = poll(&pfd, 1, 1000);  // Poll for 1 second

            if (result == -1) {
                Logger::instance().logMessage(LogLevel::ERROR, "Poll failed: " + std::string(strerror(errno)));
                return;
            }

            if (pfd.revents & POLLIN) {
                std::string message = receiveMessage();
                Logger::instance().logMessage(LogLevel::INFO, "\aReceived: \"" + message + '\"');
                if (message.empty()) {
                    Logger::instance().logMessage(LogLevel::INFO, "Client (" + getIP() + ':' + std::to_string(getPort()) + ")'s connection reset (disconnected)");
                    closeConnection();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(m_MessagePollingInterval));
        }
    });
}

void ClientConnection::stopMessagePollingThread() {
    m_MessagePolling = false;
    if (m_MessagePollingThread.joinable()) {
        m_MessagePollingThread.join();  // Wait for the thread to finish before cleaning up
    }
}

void ClientConnection::startKeepaliveThread()
{
    m_KeepaliveRunning = true;
    m_KeepaliveThread = std::thread([this] {
        Logger::instance().logMessage(LogLevel::DEBUG, "Started keepalive thread");
        while (m_KeepaliveRunning)
        {
            if (m_SocketFD != -1) {
                //Logger::instance().logMessage(LogLevel::DEBUG, "Sending keepalive message...");
                std::string keepaliveMessage = "KEEPALIVE";
                if (!keepaliveMessage.empty())
                    sendMessage("KEEPALIVE");
            }
            std::this_thread::sleep_for(std::chrono::seconds(m_KeepaliveInterval));
        }
    });
}


void ClientConnection::stopKeepaliveThread()
{
    m_KeepaliveRunning = false;
    if (m_KeepaliveThread.joinable()) {
        m_KeepaliveThread.join();  // Wait for the thread to finish before cleaning up
    }
    Logger::instance().logMessage(LogLevel::DEBUG, "Stopped keepalive thread");
}