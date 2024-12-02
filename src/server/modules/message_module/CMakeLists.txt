add_library(MessageModule STATIC
        MessageModule.cpp
        MessageModule.h
)

target_link_libraries(MessageModule
        XServerCommon
)