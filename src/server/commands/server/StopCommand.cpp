//
// Created by msullivan on 11/29/24.
//

#include "StopCommand.h"
#include <server/Server.h>

StopCommand::StopCommand()
{
    m_Name = "stop";
    m_Usage = "stop";
}

void StopCommand::execute(const std::string &args)
{
    Server::stop();
}

// extern "C" ServerCommand *importCommand()
// {
//     return new StopCommand();
// }