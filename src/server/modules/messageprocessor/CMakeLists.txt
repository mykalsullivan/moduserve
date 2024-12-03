add_library(MessageModule STATIC
        MessageProcessor.cpp
        MessageProcessor.h
)

target_link_libraries(MessageModule
        XServerCommon
)