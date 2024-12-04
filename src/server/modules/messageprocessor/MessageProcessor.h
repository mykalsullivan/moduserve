//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../ServerModule.h"
#include "server/ServerSignal.h"
#include "../networkengine/Connection.h"
#include <string>

// Forward declaration(s)
class Message;

class MessageProcessor : public ServerModule {
    friend class NetworkEngine;

public signals:
    static Signal<Connection, const std::string &> beforeMessageParse;;
    static Signal<Connection, const std::string &> processMessage;
    static Signal<Connection, const std::string &> afterMessageParse;

public slots:
    static void onProcessMessage(Connection sender, const std::string &message);

public:
    ~MessageProcessor() override = default;
    int init() override;
};
