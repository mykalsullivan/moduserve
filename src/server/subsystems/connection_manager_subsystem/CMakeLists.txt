add_library(ConnectionManagerSubsystem STATIC
        ConnectionManager.cpp
)

target_link_libraries(ConnectionManagerSubsystem
        ChatApplicationCommon
)