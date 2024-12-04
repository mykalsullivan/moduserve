add_library(ServerModules STATIC
        ServerModule.h
)

target_link_libraries(ServerModules
        XServerCommon
        ConnectionModule
        MessageModule
)

add_subdirectory(networkengine)
add_subdirectory(messageprocessor)
