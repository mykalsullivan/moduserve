add_library(Commands STATIC
        CommandManager.cpp
        CommandRegistry.cpp
)

target_link_libraries(Commands
        XServerCommon
        StopCommand
        HelpCommand
)

add_subdirectory(server/help_command)
add_subdirectory(server/stop_command)