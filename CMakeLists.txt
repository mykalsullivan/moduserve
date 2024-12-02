cmake_minimum_required(VERSION 3.25)
project(XServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)
add_compile_options(-Wall -Wextra -Os -std=c++23)

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)