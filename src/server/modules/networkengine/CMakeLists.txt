add_library(NetworkEngine SHARED
        NetworkEngine.cpp
)

target_link_libraries(NetworkEngine PUBLIC
        moduserveCommon
)