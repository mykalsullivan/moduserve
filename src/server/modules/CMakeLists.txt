add_library(Modules SHARED
        NetworkEngine.cpp
        Logger.cpp
        NetworkEngineHelperFunctions.cpp
)

target_link_libraries(Modules PUBLIC
        moduserveCommon
        BFModule
)