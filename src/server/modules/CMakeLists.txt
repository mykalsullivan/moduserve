add_library(Modules SHARED
        NetworkEngine.cpp
        NetworkEngineHelperFunctions.hpp
        Logger.cpp
        CommandProcessor.h
        CommandProcessor.cpp
)

target_link_libraries(Modules PUBLIC
        moduserveCommon
        BFModule
)