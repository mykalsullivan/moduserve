# Build and link DatabaseEngine
add_library(DatabaseEngine SHARED
        databaseengine/DatabaseEngine.cpp
)
target_link_libraries(DatabaseEngine PUBLIC
        moduserveCommon
)

# Build and link FilesystemModule
add_library(FilesystemManager SHARED
        filesystem/FilesystemModule.cpp
)
target_link_libraries(FilesystemManager PUBLIC
        moduserveCommon
)

# Build and link SSLModule
add_library(SSLModule SHARED
        ssl/SSLModule.cpp
)
target_link_libraries(SSLModule PUBLIC
        moduserveCommon
)

### EXTRA STUFF ###
# Build and link BFModule
add_library(BFModule SHARED
        bf/BFModule.cpp
        ssl/SSLModule.h
)
target_link_libraries(BFModule PUBLIC
        moduserveCommon
)