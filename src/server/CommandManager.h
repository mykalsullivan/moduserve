//
// Created by msullivan on 11/29/24.
//

class CommandManager {
public:
    CommandManager()
    {
        std::string commandLibPath = "/home/msullivan/Development/GitHub/ChatApplication/";

        // Load built-in commands
        //commandRegistry.add("stop", std::make_shared<StopCommand>());
        //commandRegistry.emplace("help", std::make_shared<HelpCommand>());

        // Load custom commands
    }
    ~CommandManager() = default;
};