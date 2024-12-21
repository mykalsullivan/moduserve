//
// Created by msullivan on 11/29/24.
//

#pragma once
#include <string>
#include <vector>
#include <any>

class ServerCommand {
protected:
    std::string m_Name;
    std::string m_Usage;
    std::vector<std::any> m_Args;

public:
    virtual ~ServerCommand() = default;
    virtual void execute([[maybe_unused]] const std::vector<std::string> &args) = 0;
    [[nodiscard]] constexpr std::string name() const { return m_Name; }
    [[nodiscard]] constexpr std::string usage() const { return m_Usage; }
};

// TODO
/* - Make a create() method in CommandManager (or somewhere) to create instances of this
 * - Delete the copy constructors
 */