//
// Created by msullivan on 12/21/24.
//

#include "NullCommand.h"
#include <server/modules/logger/Logger.h>

void NullCommand::execute(const std::vector<std::string> &args)
{
    Logger::log(LogLevel::Info, "Executed null command");
}