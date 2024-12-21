//
// Created by msullivan on 12/7/24.
//

#pragma once
#include "server/modules/ServerModule.h"
#include "server/commands/ServerCommand.h"
#include "CommandManager.h"

class CommandProcessor : public ServerModule {
public signals:
    static Signal<ServerCommand> commandAdded;

public slots:
    static void onCommandAdded(ServerCommand &command);

public:
    ~CommandProcessor() override = default;
    void init() override;
    [[nodiscard]] std::vector<std::type_index> requiredDependencies() const override { return {}; }
    [[nodiscard]] std::vector<std::type_index> optionalDependencies() const override { return {}; }

    static void registerCommand(std::shared_ptr<ServerCommand>);
    static void loadCommandFromLib(const std::string &);
   	static void execute(const std::vector<std::string> &);
};