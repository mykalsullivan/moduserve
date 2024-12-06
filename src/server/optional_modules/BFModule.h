//
// Created by msullivan on 11/8/24.
//

#pragma once
#include "server/modules/ServerModule.h"
#include <string>

class BFModule : public ServerModule {
public signals:
    static Signal<Connection, const std::string &> receivedCode;

public slots:
    static void executeCode(Connection, const std::string &);
    static void retrieveCurrentPointerAddress(Connection);
    static void retrieveCurrentPointerValue(Connection);
    static void retrieveTapeValues(Connection, unsigned int start, unsigned int offset);

public:
    ~BFModule() override;
    void init() override;
    void run() override;
    [[nodiscard]] std::vector<std::type_index> requiredDependencies() const override { return {}; };
    [[nodiscard]] std::vector<std::type_index> optionalDependencies() const override { return {}; }
};