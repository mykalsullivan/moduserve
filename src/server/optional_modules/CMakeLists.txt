# Build and link DatabaseEngine
add_library(DatabaseEngine SHARED
        DatabaseEngine.cpp
)
target_link_libraries(DatabaseEngine PUBLIC
        moduserveCommon
)

# Build and link FilesystemModule
add_library(FilesystemManager SHARED
        FilesystemModule.cpp
)
target_link_libraries(FilesystemManager PUBLIC
        moduserveCommon
)

# Build and link SSLModule
add_library(SSLModule SHARED
        SSLModule.cpp
)
target_link_libraries(SSLModule PUBLIC
        moduserveCommon
)

### EXTRA STUFF ###
# Build and link BFModule
add_library(BFModule SHARED
        BFModule.cpp
        SSLModule.h
)
target_link_libraries(BFModule PUBLIC
        moduserveCommon
)