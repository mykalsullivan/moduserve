//
// Created by msullivan on 11/10/24.
//

#pragma once
#include "../ServerModule.h"
#include "server/Signal.h"
#include <string>

// Forward declaration(s)
class OldConnection;
class Message;

class MessageModule : public ServerModule {
public:
    ~MessageModule() override = default;

public signals:
    SIGNAL(beforeParse);
    SIGNAL(afterParse);
    SIGNAL(onReceive, const OldConnection &, const std::string &);

private slots:
    SLOT(handleMessage, void, const OldConnection &sender, const std::string &message);
    SLOT(parseMessage, void, const OldConnection &sender, const std::string &message);

public:
    int init() override;
};
