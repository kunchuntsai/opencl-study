# Simple OpenCL 1.2 Print Test

cmake_minimum_required(VERSION 3.10)
project(simple_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Find OpenCL
find_package(OpenCL REQUIRED)

# Create executable
add_executable(simple_test host.c)

# Link OpenCL
target_link_libraries(simple_test OpenCL::OpenCL)

# Compiler warnings
target_compile_options(simple_test PRIVATE -Wall)

# Custom target to run the test
add_custom_target(run
    COMMAND simple_test
    DEPENDS simple_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running simple print test..."
)
