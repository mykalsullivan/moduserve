add_library(Modules STATIC
        NetworkEngine.cpp
        MessageProcessor.cpp
        Logger.cpp
)

target_link_libraries(Modules PRIVATE
        XServerCommon
)

#add_subdirectory(optional)