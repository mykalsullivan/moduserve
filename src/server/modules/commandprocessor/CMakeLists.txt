add_library(CommandProcessor SHARED
        CommandProcessor.cpp
)

target_link_libraries(CommandProcessor PUBLIC
        moduserveCommon
)