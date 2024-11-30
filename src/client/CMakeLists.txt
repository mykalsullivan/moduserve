# Define the client executable
add_executable(XServerClient
        main.cpp
        Client.cpp
        ClientConnection.cpp
)

# Link dependencies to the client executable
target_link_libraries(XServerClient
        XServerCommon
)