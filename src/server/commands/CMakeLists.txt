add_library(Commands SHARED
        CommandRegistry.cpp
)

target_link_libraries(Commands
        moduserveCommon
        ServerCommands
)

add_subdirectory(server)