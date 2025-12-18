# Simple OpenCL 1.2 Print Test

cmake_minimum_required(VERSION 3.10)
project(simple_test1 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Find OpenCL
find_package(OpenCL REQUIRED)

# Create executable
add_executable(simple_test1 host.c)

# Link OpenCL
target_link_libraries(simple_test1 OpenCL::OpenCL)

# Compiler warnings
target_compile_options(simple_test1 PRIVATE -Wall)

# Custom target to run the test
add_custom_target(run
    COMMAND simple_test1
    DEPENDS simple_test1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    COMMENT "Running simple print test..."
)
