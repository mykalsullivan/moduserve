# Define the server execuable
add_executable(ChatApplicationServer
        main.cpp
        Server.cpp
        Message.cpp
        User.cpp
        ServerConnection.cpp
)

# Link dependencies to the server executable
target_link_libraries(ChatApplicationServer
        ChatApplicationCommon
        ChatApplicationSubsystems
        ChatApplicationCommands
)

add_subdirectory(subsystems)
add_subdirectory(commands)