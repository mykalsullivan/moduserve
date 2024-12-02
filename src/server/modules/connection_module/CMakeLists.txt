add_library(ConnectionSubsystem STATIC
        ConnectionModule.cpp
        ConnectionManager.cpp
        ServerConnection.cpp
        ConnectionModule.h
        IOSystem.h
)

target_link_libraries(ConnectionSubsystem
        XServerCommon
)