# Define the server execuable
add_executable(ChatApplicationServer
        main.cpp
        Server.cpp
        Message.cpp
        User.cpp
        ServerConnection.cpp
        subsystems/ConnectionManager.cpp
        subsystems/UserManager.cpp
        subsystems/UserAuthenticator.cpp
        subsystems/BroadcastManager.cpp
        subsystems/MessageProcessor.cpp
        subsystems/CommandRegistry.cpp
)

# Link dependencies to the server executable
target_link_libraries(ChatApplicationServer
        ChatApplicationCommon
        ${PQXX_LIBRARIES}
        OpenSSL::Crypto
        OpenSSL::SSL
)