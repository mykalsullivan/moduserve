//
// Created by msullivan on 11/29/24.
//

#include "HelpCommand.h"

void HelpCommand::execute(const std::string &args)
{

}

std::string HelpCommand::usage() const
{
    return "help - Displays a list of commands (this is all it does for now)";
}

extern "C" Command *importCommand()
{
    return new HelpCommand();
}