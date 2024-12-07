add_library(BFModule SHARED
        BFModule.cpp
)

target_link_libraries(BFModule PUBLIC
        moduserveCommon
)

add_library(DatabaseEngine SHARED
        DatabaseEngine.cpp
)

target_link_libraries(DatabaseEngine PUBLIC
        moduserveCommon
)