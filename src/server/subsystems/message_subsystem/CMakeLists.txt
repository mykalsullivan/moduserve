add_library(MessageSubsystem STATIC
        MessageSubsystem.cpp
)

target_link_libraries(MessageSubsystem
        XServerCommon
)