add_library(Modules SHARED
        NetworkEngine.cpp
        NetworkEngineHelperFunctions.hpp
        Logger.cpp
        CommandProcessor.h
        CommandProcessor.cpp
        ../optional_modules/FilesystemManager.h
        ../optional_modules/FilesystemManager.cpp
)

target_link_libraries(Modules PUBLIC
        moduserveCommon
        BFModule
)