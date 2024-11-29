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
        src/server/Message.cpp
        src/server/main.cpp
        src/server/ServerConnection.cpp
        src/server/MessageHandler.cpp
        src/server/ConnectionManager.cpp
        src/server/UserManager.cpp
        src/server/UserAuthenticator.cpp
        src/User.cpp
        src/Connection.cpp
        src/Logger.cpp
)

target_link_libraries(ChatApplicationServer
        ${PQXX_LIBRARIES}
        OpenSSL::Crypto
        OpenSSL::SSL
)

add_executable(ChatApplicationClient
        src/client/main.cpp
        src/client/Client.cpp
        src/client/ClientConnection.cpp
        src/Connection.cpp
        src/Logger.cpp
        src/CommandManager.cpp
)
