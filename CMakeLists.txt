cmake_minimum_required(VERSION 3.10)
project(OpenCL_Study)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCL package
find_package(OpenCL REQUIRED)

# Include OpenCL headers
include_directories(${OpenCL_INCLUDE_DIRS})

# Add executable
add_executable(threshold_demo src/main.cpp)

# Link OpenCL library
target_link_libraries(threshold_demo ${OpenCL_LIBRARIES})

# Copy kernel files from src/cl to build/kernels directory
file(GLOB KERNEL_FILES "${CMAKE_SOURCE_DIR}/src/cl/*.cl")
file(COPY ${KERNEL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/kernels)

# Print configuration info
message(STATUS "OpenCL Include Dirs: ${OpenCL_INCLUDE_DIRS}")
message(STATUS "OpenCL Libraries: ${OpenCL_LIBRARIES}")
