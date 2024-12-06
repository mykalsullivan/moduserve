add_library(Modules STATIC
        NetworkEngine.cpp
        Logger.cpp
)

target_link_libraries(Modules PUBLIC
        moduserveCommon
        BFModule
)