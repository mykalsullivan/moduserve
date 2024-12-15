//
// Created by msullivan on 11/29/24.
//

#include "HelpCommand.h"

HelpCommand::HelpCommand()
{
    m_Name = "help";
    m_Usage = "help";
}

void HelpCommand::execute(const std::string &args)
{

}

extern "C" ServerCommand *importCommand()
{
    return new HelpCommand();
}