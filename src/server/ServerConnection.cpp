//
// Created by msullivan on 11/10/24.
//

#include "ServerConnection.h"
#include "common/PCH.h"
#include <arpa/inet.h>

ServerConnection::~ServerConnection()
{
    shutdown(m_FD, SHUT_RDWR);

    if (m_FD != -1)
    {
        close(m_FD);
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

Connection *ServerConnection::acceptClient()
{
    sockaddr_in clientAddress {};
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientFD = accept(m_FD, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
    if (clientFD < 0) return nullptr;

    auto client = new Connection();
    client->setSocket(clientFD);
    client->setAddress(clientAddress);
    client->enableKeepalive();

    return client;
}
