//
// Created by msullivan on 11/29/24.
//

#pragma once

class CommandManager {
public:
    ~CommandManager() = default;

    void loadCommand(const std::string &libPath);
};