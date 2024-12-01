add_library(Subsystems STATIC
        SubsystemManager.cpp
)

target_link_libraries(Subsystems
        XServerCommon
        ConnectionSubsystem
        MessageSubsystem
)

add_subdirectory(connection_subsystem)
add_subdirectory(message_subsystem)
add_subdirectory(database_subsystem)