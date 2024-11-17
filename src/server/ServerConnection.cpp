//
// Created by msullivan on 11/10/24.
//

#include "ServerConnection.h"
#include <iostream>
#include <arpa/inet.h>

bool ServerConnection::createAddress(int port)
{
    m_Address.sin_family = AF_INET;             // Use IPv4
    m_Address.sin_port = htons(port);           // Bind to port; include bounds checking in the future
    m_Address.sin_addr.s_addr = INADDR_ANY;     // Bind to all interfaces
    return true;
}


bool ServerConnection::bindAddress()
{
    if (bind(m_SocketFD, reinterpret_cast<sockaddr *>(&m_Address), sizeof(m_Address)) < 0)
        return false;
    return true;
}

bool ServerConnection::startListening()
{
    if (listen(m_SocketFD, 5) < 0)
        return false;
    return true;
}

bool ServerConnection::acceptClient(Connection &client)
{
    auto clientSocketAddress = reinterpret_cast<sockaddr *>(client.getSocket());
    ssize_t addressLength = sizeof(client);

    int clientSocket = accept(m_SocketFD, clientSocketAddress, reinterpret_cast<socklen_t *>(&addressLength));
    if (clientSocket < 0)
        return false;

    client.setSocket(clientSocket);
    client.setAddress(m_Address);
    return true;
}
