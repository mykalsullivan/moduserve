add_library(Modules SHARED
        NetworkEngine.cpp
        Logger.cpp
)

target_link_libraries(Modules PUBLIC
        moduserveCommon
        BFModule
)