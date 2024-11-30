# Create a static library for the common code
add_library(ChatApplicationCommon STATIC
        Connection.cpp
        Logger.cpp
        PCH.cpp
)

# Set the include directories for the static library
target_include_directories(ChatApplicationCommon PUBLIC
        ${PROJECT_SOURCE_DIR}/src       # Include the main source directory for other headers
)

# Add the precompiled headers file to the common library
target_precompile_headers(ChatApplicationCommon PUBLIC
        PCH.h
)