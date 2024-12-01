add_library(Commands STATIC
        CommandManager.cpp
        CommandRegistry.cpp
)

target_link_libraries(Commands
        XServerCommon
        StopCommand
        HelpCommand
)

add_subdirectory(help_command)
add_subdirectory(stop_command)