cmake_minimum_required(VERSION 3.25)
project(ChatApplication)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
find_package(OpenSSL REQUIRED)
include_directories(${PQXX_INCLUDE_DIRS})
link_directories(${PQXX_LIBRARY_DIRS})

add_compile_options(-Wall -Wextra -Os)

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)