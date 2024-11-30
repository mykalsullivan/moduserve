add_library(CommandSubsystem STATIC
        CommandSubsystem.cpp
)

target_link_libraries(CommandSubsystem
        XServerCommon
)