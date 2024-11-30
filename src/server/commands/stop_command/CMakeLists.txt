add_library(StopCommand STATIC
        StopCommand.cpp
)

target_link_libraries(StopCommand
        ChatApplicationCommon
)
