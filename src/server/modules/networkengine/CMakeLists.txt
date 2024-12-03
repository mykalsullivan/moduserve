add_library(ConnectionModule STATIC
        NetworkEngine.cpp
        NetworkEngine.h
)

target_link_libraries(ConnectionModule
        XServerCommon
)