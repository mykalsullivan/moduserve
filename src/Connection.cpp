//
// Created by msullivan on 11/9/24.
//

#include "Connection.h"
#include "server/Logger.h"
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

Connection::Connection()
    : m_SocketFD(-1), m_Address()
{
    memset(&m_Address, 0, sizeof(m_Address));
}

Connection::~Connection()
{
    shutdown(m_SocketFD, SHUT_RDWR);

    if (m_SocketFD != -1) {
        close(m_SocketFD);
        m_SocketFD = -1;
    }
}

bool Connection::createSocket()
{
    m_SocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (m_SocketFD < 0) {
        Logger::instance().logMessage(LogLevel::ERROR, m_SocketFD + ": Socket creation failed");
        return false;
    }
    return true;
}

void Connection::setSocket(int fd)
{
    m_SocketFD = fd;
}

void Connection::setAddress(sockaddr_in address)
{
    m_Address = address;
}

std::string Connection::getIP() const {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(m_Address.sin_addr), ipStr, INET_ADDRSTRLEN);
    return std::string(ipStr);
}

int Connection::getPort() const {
    return ntohs(m_Address.sin_port);
}

void Connection::setNonBlocking() const
{
    if (m_SocketFD == -1)
    {
        Logger::instance().logMessage(LogLevel::ERROR, "Socket is invalid");
        return;
    }

    int flags = fcntl(m_SocketFD, F_GETFL);
    if (flags == -1)
    {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to get socket flags: " + std::to_string(errno));
        return;
    }

    // Set the socket to non-blocking mode
    if (fcntl(m_SocketFD, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        Logger::instance().logMessage(LogLevel::ERROR, "Failed to set socket to non-blocking mode: " + std::to_string(errno));
        return;
    }
    Logger::instance().logMessage(LogLevel::DEBUG, "Socket set to non-blocking mode");
}

void Connection::enableKeepalive() const
{
    int enableKeepalive = 1;
    setsockopt(m_SocketFD, SOL_SOCKET, SO_KEEPALIVE, &enableKeepalive, sizeof(enableKeepalive));
    Logger::instance().logMessage(LogLevel::DEBUG, "Keepalive enabled on connection " + m_SocketFD);
}


bool Connection::sendMessage(const std::string &message) const
{
    //Logger::instance().logMessage(LogLevel::DEBUG, "Attempting to send message: " + message);
    if (m_SocketFD != -1 && !message.empty()) {
        ssize_t bytesSent = send(m_SocketFD, message.c_str(), message.length(), 0);

        if (bytesSent == -1) {
            //Logger::instance().logMessage(LogLevel::ERROR, "Error sending message: " + std::string(strerror(errno)));
            return false;
        }
        //Logger::instance().logMessage(LogLevel::DEBUG, "Sent message: " + message);
        return true;
    }
    //Logger::instance().logMessage(LogLevel::DEBUG, "Attempted to send an empty message");
    return false;
}

std::string Connection::receiveMessage()
{
    char buffer[1024];
    ssize_t bytesReceived = recv(m_SocketFD, buffer, sizeof(buffer), 0);

    // Handle the data where the connection was reset or closed
    if (bytesReceived == 0) {
        Logger::instance().logMessage(LogLevel::INFO, "Connection closed by peer");
        return "";
    }
    if (bytesReceived < 0) {
        Logger::instance().logMessage(LogLevel::ERROR, "Error receiving data on connection");
        return "";
    }

    //Logger::instance().logMessage(LogLevel::INFO, "(receiveMessage()) received: " + std::string(buffer, bytesReceived));

    updateLastActivityTime();
    return std::string(buffer, bytesReceived); // Create string from buffer
}

bool Connection::isValid() const
{
    char buffer;
    ssize_t result = recv(m_SocketFD, &buffer, 1, MSG_PEEK);
    if (result == 0) {
        // Connection was closed by the other side
        return false;
    }
    if (result < 0) {
        // If errno is EAGAIN or EWOULDBLOCK, it's just a non-blocking operation
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            //Logger::instance().logMessage(LogLevel::DEBUG, "Non-blocking operation");
            return true;
        }
        // Check for other errors (e.g., ECONNRESET or EPIPE)
        return false;
    }
    return true;
}


bool Connection::isInactive(int timeout) const
{
    auto currentTime = std::chrono::steady_clock::now();
    return (currentTime - m_LastActivityTime) > std::chrono::seconds(timeout);
}

void Connection::updateLastActivityTime()
{
    m_LastActivityTime = std::chrono::steady_clock::now();
}