add_library(CommandSubsystem STATIC
        CommandSubsystem.cpp
        CommandRegistry.cpp
)

target_link_libraries(CommandSubsystem
        XServerCommon
)