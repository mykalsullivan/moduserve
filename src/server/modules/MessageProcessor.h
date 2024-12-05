//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "ServerModule.h"
#include "server/Signal.h"
#include <string>

// Forward declaration(s)
class Message;

class MessageProcessor : public ServerModule {
    friend class NetworkEngine;

public signals:
    static Signal<Connection, const std::string &> receivedCommand;

public slots:
    static void processMessage(Connection sender, const std::string &message);

public:
    ~MessageProcessor() override = default;
    void init() override;
    void run() override {}
    [[nodiscard]] std::vector<std::type_index> requiredDependencies() const override { return {}; }
    [[nodiscard]] std::vector<std::type_index> optionalDependencies() const override { return {}; }
};
