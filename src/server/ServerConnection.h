//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../Connection.h"

class ServerConnection : public Connection {
public:
    ServerConnection() = default;
    ~ServerConnection() override = default;

    bool createAddress(int port);
    bool bindAddress();
    bool startListening();
    bool acceptClient(Connection &connection);
};