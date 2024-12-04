add_library(HelpCommand STATIC
        HelpCommand.cpp
)

target_link_libraries(HelpCommand
        XServerCommon
)