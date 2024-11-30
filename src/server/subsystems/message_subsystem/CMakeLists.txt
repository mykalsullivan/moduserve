add_library(MessageProcessorSubsystem STATIC
        MessageProcessor.cpp
)

target_link_libraries(MessageProcessorSubsystem
        ChatApplicationCommon
)