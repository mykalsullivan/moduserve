# Define the server execuable
add_executable(XServer
        main.cpp
        Server.cpp
)

# Link dependencies to the server executable
target_link_libraries(XServer
        XServerCommon
        ServerModules
        Commands
        OptionalServerModules
)

#target_link_libraries(XServerCommon PRIVATE
#        ws2_32
#)

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
    #include <mutex>
    int main() {
        std::mutex m;
        return 0;
    }
" HAVE_STD_MUTEX)

if(NOT HAVE_STD_MUTEX)
    message(FATAL_ERROR "std::mutex is not supported by the compiler.")
endif()

add_subdirectory(modules)
add_subdirectory(commands)