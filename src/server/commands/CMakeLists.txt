add_library(Commands STATIC
        CommandRegistry.cpp
)

target_link_libraries(Commands
        moduserveCommon
        ServerCommands
)

add_subdirectory(server)