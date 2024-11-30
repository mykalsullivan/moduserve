# Create a static library for the common code
add_library(ChatApplicationCommon STATIC
        Connection.cpp
        Logger.cpp
)

# Set the include directories for the static library
target_include_directories(ChatApplicationCommon PUBLIC
        ${PROJECT_SOURCE_DIR}/src       # Include the main source directory for other headers
)