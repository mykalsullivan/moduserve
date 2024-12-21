add_library(Logger SHARED
        Logger.cpp
)

target_link_libraries(Logger PUBLIC
        moduserveCommon
)