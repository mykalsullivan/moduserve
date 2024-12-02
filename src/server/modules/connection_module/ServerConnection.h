//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "common/Connection.h"

class ServerConnection : public OldConnection {
public:
    ServerConnection() = default;
    ~ServerConnection() override;

    bool createAddress(int port);
    bool bindAddress();
    bool startListening();
    OldConnection *acceptClient();
};