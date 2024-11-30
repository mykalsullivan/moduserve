cmake_minimum_required(VERSION 3.25)
project(ChatApplication)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -Wextra -Os)

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)