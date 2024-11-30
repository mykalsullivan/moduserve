//
// Created by msullivan on 11/29/24.
//

#include "StopCommand.h"
#include "server/Server.h"

void StopCommand::execute(const std::string &args)
{
    Server::instance().stop();
}

extern "C" Command *importCommand()
{
    return new StopCommand();
}