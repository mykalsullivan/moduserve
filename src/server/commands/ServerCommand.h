//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <string>
#include <any>

class ServerCommand {
protected:
    std::string m_Action;
    std::vector<std::any> m_Args;
public:
    virtual ~ServerCommand() = default;
    virtual void execute() = 0;
    [[nodiscard]] constexpr virtual std::string name() const = 0;
    [[nodiscard]] constexpr virtual std::string usage() const = 0;
};

// TODO
/* - Make a create() method in CommandManager (or somewhere) to create instances of this
 * - Delete the copy constructors
 */