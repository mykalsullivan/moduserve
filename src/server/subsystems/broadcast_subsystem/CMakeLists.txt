add_library(BroadcastSubsystem STATIC
        BroadcastManager.cpp
)

target_link_libraries(BroadcastSubsystem
        ChatApplicationCommon
)