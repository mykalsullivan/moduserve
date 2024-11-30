# Define the client executable
add_executable(ChatApplicationClient
        main.cpp
        Client.cpp
        ClientConnection.cpp
)

# Link dependencies to the client executable
target_link_libraries(ChatApplicationClient
        ChatApplicationCommon
)