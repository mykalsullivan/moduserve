add_library(Subsystems STATIC
        ServerModuleManager.cpp
        ServerModule.h
        ../TEMP_optional_modules/user_module/UserModule.h
)

target_link_libraries(Subsystems
        XServerCommon
        ConnectionSubsystem
        MessageSubsystem
)

add_subdirectory(connection_module)
add_subdirectory(message_module)
