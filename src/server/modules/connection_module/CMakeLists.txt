add_library(ConnectionModule STATIC
        ConnectionModule.cpp
        ConnectionManager.cpp
        ServerConnection.cpp
        ConnectionModule.h
        IOSystem.h
)

target_link_libraries(ConnectionModule
        XServerCommon
)