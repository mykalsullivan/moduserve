add_library(BroadcastSubsystem STATIC
        BroadcastSubsystem.cpp
)

target_link_libraries(BroadcastSubsystem
        ChatApplicationCommon
)