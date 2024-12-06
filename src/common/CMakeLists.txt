# Create a static library for the common code
add_library(moduserveCommon STATIC
        PCH.cpp
        Message.cpp
)

# Set the include directories for the static library
target_include_directories(moduserveCommon PUBLIC
        ..       # Include the main source directory for other headers
)

# Add the precompiled headers file to the common library
target_precompile_headers(moduserveCommon PUBLIC
        PCH.h
)