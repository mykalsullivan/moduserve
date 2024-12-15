//
// Created by msullivan on 11/29/24.
//

#include "StopCommand.h"

StopCommand::StopCommand() : m_Name("stop")
{}

void StopCommand::execute(const std::string &args)
{

}

extern "C" ServerCommand *importCommand()
{
    return new StopCommand();
}