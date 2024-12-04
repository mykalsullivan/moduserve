add_library(BFModule STATIC
        BFModule.cpp
)

target_link_libraries(BFModule
        XServerCommon
)