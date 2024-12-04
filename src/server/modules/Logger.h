//
// Created by msullivan on 12/4/24.
//

#pragma once
#include "ServerModule.h"

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger : public ServerModule {
public:
    virtual ~Logger();
    void init() override;
    void run() override;
    std::vector<std::type_index> requiredDependencies() const override { return {}; };
    std::vector<std::type_index> optionalDependencies() const override { return {}; };

    static void log(LogLevel level = LogLevel::Info, const std::string &message = "(empty)");
};
