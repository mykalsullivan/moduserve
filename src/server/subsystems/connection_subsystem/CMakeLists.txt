add_library(ConnectionSubsystem STATIC
        ConnectionSubsystem.cpp
        ConnectionManager.cpp
        ServerConnection.cpp
)

target_link_libraries(ConnectionSubsystem
        XServerCommon
)