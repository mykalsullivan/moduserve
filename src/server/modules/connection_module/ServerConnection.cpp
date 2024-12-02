//
// Created by msullivan on 11/10/24.
//

#include "ServerConnection.h"
#include "common/PCH.h"

#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

ServerConnection::~ServerConnection()
{
    // Needs a cross-platform version
    //shutdown(m_FD, SHUT_RDWR);

    if (m_FD != -1)
    {
#ifndef _WIN32
        close(m_FD);
#else
        closesocket(m_FD);
#endif
        m_FD = -1;
    }
}

bool ServerConnection::createAddress(int port)
{
    m_Address.sin_family = AF_INET;             // Use IPv4
    m_Address.sin_port = htons(port);           // Bind to port; include bounds checking in the future
    m_Address.sin_addr.s_addr = INADDR_ANY;     // Bind to all interfaces
    return true;
}


bool ServerConnection::bindAddress()
{
    if (bind(m_FD, reinterpret_cast<sockaddr *>(&m_Address), sizeof(m_Address)) >= 0) return true;
    return false;
}

bool ServerConnection::startListening()
{
    if (listen(m_FD, 5) >= 0) return true;
    return false;
}

OldConnection *ServerConnection::acceptClient()
{
    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientFD = accept(m_FD, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
    if (clientFD < 0) return nullptr;

    auto client = new OldConnection();
    client->setSocket(clientFD);
    client->setAddress(clientAddress);
    return client;
}
