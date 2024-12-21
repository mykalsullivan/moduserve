//
// Created by msullivan on 12/7/24.
//

#pragma once
#include "server/modules/ServerModule.h"
#include "server/commands/ServerCommand.h"
#include "CommandManager.h"

class CommandProcessor : public ServerModule {
public signals:
    static Signal<const std::string &> commandAdded;
    static Signal<Connection, const std::string &> receivedInvalidCommand;

public slots:
    static void onCommandAdded(const std::string &commandName);
    static void onReceivedInvalidCommand(Connection connection, const std::string &);

public:
    ~CommandProcessor() override = default;
    void init() override;
    [[nodiscard]] std::vector<std::type_index> requiredDependencies() const override { return {}; }
    [[nodiscard]] std::vector<std::type_index> optionalDependencies() const override { return {}; }

    static void registerCommand(std::shared_ptr<ServerCommand>);
    static void loadCommandFromLib(const std::string &);
    static std::vector<std::string> stringToVec(const std::string &);
    static std::string vecToString(const std::vector<std::string> &);
   	static void execute(Connection connection, const std::string &);
};