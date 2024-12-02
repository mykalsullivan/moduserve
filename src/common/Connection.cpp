//
// Created by msullivan on 11/9/24.
//

#include "Connection.h"
#include "PCH.h"
#include <fcntl.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#else
#include <ws2tcpip.h>
#endif

OldConnection::OldConnection()
    : m_FD(-1), m_Address()
{
    memset(&m_Address, 0, sizeof(m_Address));
    updateLastActivityTime();
}

OldConnection::~OldConnection()
{
    // I need to find a cross-platform version of this
    //shutdown(m_FD, SHUT_RDWR);

    if (m_FD != -1)
    {
#ifndef WIN32
        close(m_FD);
#else
        closesocket(m_FD);
#endif
        m_FD = -1;
    }
}

bool OldConnection::createSocket()
{
    m_FD = socket(AF_INET, SOCK_STREAM, 0);
    if (m_FD >= 0)
    {
        // Set to non-blocking mode, then return true if that works
#ifdef WIN32
        u_long mode = 1; // 1 = non-blocking
        if (ioctlsocket(m_FD, FIONBIO, &mode) != 0)
        {
            logMessage(LogLevel::Error, "Failed to to set socket to non-blocking mode");
            return false;
        }
#else
        int flags = fcntl(m_FD, F_GETFL, 0);
        if (flags == -1 || fcntl(m_FD, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            logMessage(LogLevel::Error, "Failed to to set socket to non-blocking mode");
            return false;
        }
#endif
        // Enable keepalive
        int enableKeepalive = 1;
        setsockopt(m_FD, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char *>(&enableKeepalive), sizeof(enableKeepalive));
        return true;
    }
#ifdef WIN32
    logMessage(LogLevel::Fatal, "Socket creation failed: " + std::to_string(WSAGetLastError()));
#else
    logMessage(LogLevel::Fatal, "Socket creation failed: " + std::string(strerror(errno)));
#endif
    return false;
}

void OldConnection::setSocket(int fd)
{
    m_FD = fd;
}

void OldConnection::setAddress(sockaddr_in address)
{
    m_Address = address;
}

std::string OldConnection::ip() const {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(m_Address.sin_addr), ipStr, INET_ADDRSTRLEN);
    return std::string(ipStr);
}

int OldConnection::port() const {
    return ntohs(m_Address.sin_port);
}

bool OldConnection::sendData(const std::string &data) const
{
    if (m_FD != -1 && !data.empty())
    {
        ssize_t bytesSent = send(m_FD, data.c_str(), data.length(), 0);

        if (bytesSent == -1)
        {
#ifdef WIN32
            logMessage(LogLevel::Error, "Error sending data: \"" + std::to_string(WSAGetLastError()) + '\"');
#else
            logMessage(LogLevel::Error, "Error sending data: \"" + std::string(strerror(errno)) + '\"');
#endif
            return false;
        }
        return true;
    }
    logMessage(LogLevel::Debug, "Attempted to send an empty data");
    return false;
}

std::string OldConnection::receiveData()
{
    // Check if there is pending data and return if no data is available
    if (!hasPendingData()) return "";

    char buffer[1024] {};
    ssize_t bytesReceived = recv(m_FD, buffer, sizeof(buffer), 0);

    // Handle errors
    if (bytesReceived <= 0)
    {
#ifdef WIN32
        if (bytesReceived == 0)
            logMessage(LogLevel::Info, "Connection closed by peer");
        else
            logMessage(LogLevel::Error, "Error receiving data: \"" + std::to_string(WSAGetLastError()) + '\"');
#else
        if (bytesReceived == 0)
            logMessage(LogLevel::Info, "Connection closed by peer");
        else
            logMessage(LogLevel::Error, "Error receiving data: \"" + std::string(strerror(errno)) + '\"');
#endif
        return "";
    }

    updateLastActivityTime();
    return std::string(buffer, bytesReceived); // Create string from buffer
}

bool OldConnection::isValid() const
{
    // Check if there is pending data and return if no data is available
    if (!hasPendingData()) return true;

    char buffer[1] {};
    ssize_t result = recv(m_FD, buffer, sizeof(buffer), MSG_PEEK);

    if (result < 0)
    {
        // If errno is EAGAIN or EWOULDBLOCK, it's just a non-blocking operation
#ifdef WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
        {
            //logMessage(LogLevel::DEBUG, "Non-blocking operation");
            return true;
        }
        // Check for other errors (e.g., ECONNRESET or EPIPE)
        return false;
    }
    // Connection was closed by peer
    if (result == 0) return false;
    return true;
}

bool OldConnection::isInactive(int timeout) const
{
    auto currentTime = std::chrono::steady_clock::now();
    return (currentTime - m_LastActivityTime) > std::chrono::seconds(timeout);
}


bool OldConnection::hasPendingData() const
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

void OldConnection::updateLastActivityTime()
{
    m_LastActivityTime = std::chrono::steady_clock::now();
}