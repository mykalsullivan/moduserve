add_library(ConnectionSubsystem STATIC
        ConnectionSubsystem.cpp
        ConnectionManager.cpp
)

target_link_libraries(ConnectionSubsystem
        XServerCommon
)