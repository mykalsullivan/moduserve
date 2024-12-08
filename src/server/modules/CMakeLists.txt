add_library(Modules SHARED
        NetworkEngine.cpp
        NetworkEngineHelperFunctions.hpp
        Logger.cpp
        CommandProcessor.h
        CommandProcessor.cpp
        ../optional_modules/FilesystemModule.h
        ../optional_modules/FilesystemModule.cpp
)

target_link_libraries(Modules PUBLIC
        moduserveCommon
        BFModule
)