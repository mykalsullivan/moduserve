add_library(CommandRegistrySubsystem STATIC
        CommandRegistry.cpp
)

target_link_libraries(CommandRegistrySubsystem
        ChatApplicationCommon
)