//
// Created by msullivan on 11/9/24.
//

#include "Connection.h"
#include "Logger.h"
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

Connection::Connection()
    : m_FD(-1), m_Address()
{
    memset(&m_Address, 0, sizeof(m_Address));
}

Connection::~Connection()
{
    shutdown(m_FD, SHUT_RDWR);

    if (m_FD != -1)
    {
        close(m_FD);
        m_FD = -1;
    }
}

bool Connection::createSocket()
{
    m_FD = socket(AF_INET, SOCK_STREAM, 0);
    if (m_FD >= 0)
    {
        // Set to non-blocking mode, then return true if that works
        int flags = fcntl(m_FD, F_GETFL, 0);
        if (flags == -1)
        {
            LOG(LogLevel::ERROR, "Failed to get socket flags");
            return false;
        }
        if (fcntl(m_FD, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            LOG(LogLevel::ERROR, "Failed to to set socket to non-blocking mode");
            return false;
        }
        return true;
    }
    LOG(LogLevel::ERROR, m_FD + ": Socket creation failed");
    return false;
}

void Connection::setSocket(int fd)
{
    m_FD = fd;
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

void Connection::enableKeepalive() const
{
    int enableKeepalive = 1;
    setsockopt(m_FD, SOL_SOCKET, SO_KEEPALIVE, &enableKeepalive, sizeof(enableKeepalive));
}


bool Connection::sendData(const std::string &data) const
{
    //LOG(LogLevel::DEBUG, "Attempting to send message: " + message);
    if (m_FD != -1 && !data.empty())
    {
        ssize_t bytesSent = send(m_FD, data.c_str(), data.length(), 0);

        if (bytesSent == -1)
        {
            //LOG(LogLevel::ERROR, "Error sending message: " + std::string(strerror(errno)));
            return false;
        }
        //LOG(LogLevel::DEBUG, "Sent message: " + message);
        return true;
    }
    //LOG(LogLevel::DEBUG, "Attempted to send an empty message");
    return false;
}

std::string Connection::receiveData()
{
    // Check if there is pending data and return if no data is available
    if (!hasPendingData()) return "";

    char buffer[1024] {};
    ssize_t bytesReceived = recv(m_FD, buffer, sizeof(buffer), 0);

    // Handle errors
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
            LOG(LogLevel::INFO, "Connection closed by peer")
        else
            LOG(LogLevel::ERROR, "Error receiving data on connection")
        return "";
    }

    updateLastActivityTime();
    return std::string(buffer, bytesReceived); // Create string from buffer
}

bool Connection::isValid() const
{
    // Check if there is pending data and return if no data is available
    if (!hasPendingData()) return true;

    char buffer[1] {};
    ssize_t result = recv(m_FD, buffer, sizeof(buffer), MSG_PEEK);

    if (result < 0)
    {
        // If errno is EAGAIN or EWOULDBLOCK, it's just a non-blocking operation
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            //LOG(LogLevel::DEBUG, "Non-blocking operation");
            return true;
        }
        // Check for other errors (e.g., ECONNRESET or EPIPE)
        return false;
    }
    // Connection was closed by the other side
    if (result == 0) return false;
    return true;
}

bool Connection::hasPendingData() const
{
    if (m_FD == -1) return false;
    fd_set readFDs;
    timeval timeout = { 0, 0 };

    FD_ZERO(&readFDs);      // Clear FD set
    FD_SET(m_FD, &readFDs); // Add m_FD to the set

    // Return true if there is pending data
    int result = select(m_FD + 1, &readFDs, nullptr, nullptr, &timeout);
    if (result > 0 && FD_ISSET(m_FD, &readFDs)) return true;

    // Return false if there is either an error or there is no pending data
    return false;
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