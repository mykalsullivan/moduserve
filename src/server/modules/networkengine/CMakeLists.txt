add_library(ConnectionModule STATIC
        ConnectionEngine.cpp
        ConnectionEngine.h
)

target_link_libraries(ConnectionModule
        XServerCommon
)