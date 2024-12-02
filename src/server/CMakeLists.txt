# Define the server execuable
add_executable(XServer
        main.cpp
        Server.cpp
)

# Link dependencies to the server executable
target_link_libraries(XServer
        XServerCommon
        Subsystems
        Commands
)

add_subdirectory(subsystems)
add_subdirectory(commands)