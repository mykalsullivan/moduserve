add_library(ConnectionSubsystem STATIC
        ConnectionSubsystem.cpp
)

target_link_libraries(ConnectionSubsystem
        XServerCommon
)