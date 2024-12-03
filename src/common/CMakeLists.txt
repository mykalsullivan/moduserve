# Create a static library for the common code
add_library(XServerCommon STATIC
        PCH.cpp
        Logger.cpp
        Message.cpp
)

# Set the include directories for the static library
target_include_directories(XServerCommon PUBLIC
        ${PROJECT_SOURCE_DIR}/src       # Include the main source directory for other headers
)

# Add the precompiled headers file to the common library
target_precompile_headers(XServerCommon PUBLIC
        PCH.h
)