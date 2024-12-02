add_library(ServerModules STATIC
        ServerModuleManager.cpp
        ServerModule.h
        ../TEMP_optional_modules/user_module/UserModule.h
)

target_link_libraries(ServerModules
        XServerCommon
        ConnectionModule
        MessageModule
)

add_subdirectory(connection_module)
add_subdirectory(message_module)
