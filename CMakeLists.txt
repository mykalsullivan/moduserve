cmake_minimum_required(VERSION 3.25)
project(ChatApplication)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

find_package(OpenSSL REQUIRED)

include_directories(${PQXX_INCLUDE_DIRS})
link_directories(${PQXX_LIBRARY_DIRS})

add_executable(ChatApplicationServer
        src/server/main.cpp
        src/server/Server.cpp
        src/server/Server.h
        src/server/Message.cpp
        src/server/Message.h
        src/User.h
        src/User.cpp
        src/Connection.h
        src/Connection.cpp
        src/server/main.cpp
        src/server/ServerConnection.h
        src/server/ServerConnection.cpp
        src/server/Logger.h
        src/server/Logger.cpp
        src/server/MessageHandler.h
        src/server/MessageHandler.cpp
        src/server/ConnectionManager.h
        src/server/ConnectionManager.cpp
        src/server/UserManager.h
        src/server/UserManager.cpp
        src/server/UserAuthenticator.cpp
        src/server/AdminPrompt.h
        src/server/AdminPrompt.cpp
)

target_link_libraries(ChatApplicationServer
        ${PQXX_LIBRARIES}
        OpenSSL::Crypto
        OpenSSL::SSL
)

add_executable(ChatApplicationClient
        src/client/main.cpp
        src/client/Client.cpp
        src/Connection.cpp
        src/CommandLineArgs.h
        src/client/ClientConnection.h
        src/client/ClientConnection.cpp
        src/Logger.cpp
        src/server/UserAuthenticator.h
        src/UUID.h
        src/Role.h
        src/CommandManager.h
        src/CommandManager.cpp
)
