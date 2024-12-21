add_library(ServerCommands STATIC
        StopCommand.cpp
        HelpCommand.cpp
        NullCommand.cpp
)

target_link_libraries(ServerCommands
        moduserveCommon
)