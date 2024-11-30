//
// Created by msullivan on 11/29/24.
//

#include "HelpCommand.h"
#include "../Server.h"

void HelpCommand::execute(const std::string &args)
{

}

extern "C" Command *importCommand()
{
    return new HelpCommand();
}