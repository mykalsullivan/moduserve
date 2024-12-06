add_library(BFModule
        BFModule.cpp
)

target_link_libraries(BFModule PUBLIC
        moduserveCommon
)

add_library(DatabaseEngine
        DatabaseEngine.cpp
)

target_link_libraries(DatabaseEngine PUBLIC
        moduserveCommon
)