add_library(UserSubsystem STATIC
        UserAuthenticationSubsystem.cpp
        UserSubsystem.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
find_package(OpenSSL REQUIRED)
include_directories(${PQXX_INCLUDE_DIRS})
link_directories(${PQXX_LIBRARY_DIRS})

target_link_libraries(UserSubsystem
        XServerCommon
        ${PQXX_LIBRARIES}
        OpenSSL::Crypto
        OpenSSL::SSL
)