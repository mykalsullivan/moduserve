# Define the server execuable
add_executable(XServer
        main.cpp
        Server.cpp
        Message.cpp
        User.cpp
        ServerConnection.cpp
)

# Link dependencies to the server executable
target_link_libraries(XServer
        XServerCommon
        ConnectionSubsystem
        MessageSubsystem
        BroadcastSubsystem
        CommandSubsystem
        UserSubsystem
)

add_subdirectory(subsystems)
add_subdirectory(commands)