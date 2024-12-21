add_library(ServerCommands SHARED
        StopCommand.cpp
        HelpCommand.cpp
)

target_link_libraries(ServerCommands
        moduserveCommon
)