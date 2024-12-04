add_library(ConnectionModule STATIC
        NetworkEngine.cpp
)

target_link_libraries(ConnectionModule
        XServerCommon
)