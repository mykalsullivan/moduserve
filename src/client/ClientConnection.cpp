//
// Created by msullivan on 11/10/24.
//

#include "ClientConnection.h"
#include "../common/Logger.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <poll.h>

ClientConnection::~ClientConnection()
{
    if (m_FD != -1)
    {
        shutdown(m_FD, SHUT_RDWR);
        close(m_FD);
    }
    stopKeepaliveThread();
    stopMessagePollingThread();
}

bool ClientConnection::createAddress(const std::string& ip, int port)
{
    m_Address.sin_family = AF_INET;         // Use IPv4
    m_Address.sin_port = htons(port);       // Bind port

    // Convert IP address from string to socket address
    if (inet_pton(AF_INET, ip.c_str(), &m_Address.sin_addr) <= 0) {
        LOG(LogLevel::ERROR, "Failed to convert IP address from string");
        return false;
    }
    return true;
}

int ClientConnection::connectToServer()
{
    LOG(LogLevel::DEBUG, "Attempting to connect to server at " + getIP() + ':' + std::to_string(getPort()));

    // Attempt to connect
    int connectionResult = connect(m_FD, reinterpret_cast<sockaddr *>(&m_Address), sizeof(m_Address));

    // Immediately return if successful
    if (connectionResult == 0) {
        LOG(LogLevel::DEBUG, "Connection successful");
        return 1;
    }

    // Handle errors
    if (connectionResult == -1) {
        // Handle errors or retry based on the errno
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No pending connections, can retry after a short delay or poll
            std::cout << "No incoming connections right now\n";
            LOG(LogLevel::ERROR, "No incoming connections right now");
            return -1;
        }
        else if (errno == EINTR) {
            // Interrupted by signal, retry
            LOG(LogLevel::ERROR, "Interrupted by a signal");
            return -1;
        }
        else if (errno == ECONNREFUSED) {
            LOG(LogLevel::ERROR, "Connection refused");
            return -1;
        }
        else if (errno == ECONNABORTED) {
            LOG(LogLevel::ERROR, "Connection aborted");
            return -1;
        }
    }

    // If connection is in progress, use poll to wait for it
    if (connectionResult == -1 && errno == EINPROGRESS) {
        pollfd pfd {};
        pfd.fd = m_FD;
        pfd.events = POLLOUT; // We are waiting for the socket to become writable (i.e., connected)
        pfd.revents = 0;

        int result = poll(&pfd, 1, 15000); // 30-second timeout
        LOG(LogLevel::DEBUG, "Poll result: " + std::to_string(result));

        if (result <= 0) {
            LOG(LogLevel::ERROR, "Connection timed out or poll failed");
            return 0;
        }

        // If poll() succeeds, check if the connection was successful
        if (pfd.revents & POLLOUT) {
            // Check if the socket is writable (connection is completed)
            int socketError = 0;
            socklen_t len = sizeof(socketError);
            if (getsockopt(m_FD, SOL_SOCKET, SO_ERROR, &socketError, &len) == -1) {
                LOG(LogLevel::ERROR, "Failed to check socket error after poll");
                return -2;
            }

            // Connection failed for another reason
            if (socketError != 0) {
                LOG(LogLevel::ERROR, "Connection failed: " + socketError);

                return -3;
            }
            return 1;
        }
    }
    LOG(LogLevel::ERROR, "Failed to connect to server: error " + std::to_string(errno));
    return -1;
}

void ClientConnection::closeConnection()
{
    if (m_FD != -1) {
        LOG(LogLevel::INFO, "Closing connection to " + getIP() + ':' + std::to_string(getPort()));

        close(m_FD);
        m_FD = -1;
    }
    stopMessagePollingThread();
    stopKeepaliveThread();
}


std::string ClientConnection::receiveMessage() const
{
    char buffer[1024];
    ssize_t bytesReceived = recv(m_FD, buffer, sizeof(buffer), 0);
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
            //LOG(LogLevel::DEBUG, "Polling...");
            pfd.fd = m_FD;       // Set the file descriptor
            pfd.events = POLLIN;       // Make it poll events
            pfd.revents = 0;           // Initialize the events to 0

            int result = poll(&pfd, 1, 1000);  // Poll for 1 second

            if (result == -1) {
                LOG(LogLevel::ERROR, "Poll failed: " + std::string(strerror(errno)));
                return;
            }

            if (pfd.revents & POLLIN) {
                std::string message = receiveMessage();
                LOG(LogLevel::INFO, "\aReceived: \"" + message + '\"');
                if (message.empty()) {
                    LOG(LogLevel::INFO, "Client (" + getIP() + ':' + std::to_string(getPort()) + ")'s connection reset (disconnected)");
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
        LOG(LogLevel::DEBUG, "Started keepalive thread");
        while (m_KeepaliveRunning)
        {
            if (m_FD != -1) {
                //LOG(LogLevel::DEBUG, "Sending keepalive message...");
                std::string keepaliveMessage = "KEEPALIVE";
                if (!keepaliveMessage.empty())
                    sendData("KEEPALIVE");
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
    LOG(LogLevel::DEBUG, "Stopped keepalive thread");
}