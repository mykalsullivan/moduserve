//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../ServerModule.h"
#include "server/ServerSignals.h"
#include "../networkengine/Connection.h"
#include <string>

// Forward declaration(s)
class Message;

class MessageProcessor : public ServerModule {
public signals:
    static void beforeParse() {}
    static void afterParse() {}

private slots:
    static void handleMessage(Connection sender, const std::string &message);

public:
    ~MessageProcessor() override = default;
    int init() override;
};
