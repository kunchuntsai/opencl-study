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

# Copy kernels directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/kernels DESTINATION ${CMAKE_BINARY_DIR})

# Print configuration info
message(STATUS "OpenCL Include Dirs: ${OpenCL_INCLUDE_DIRS}")
message(STATUS "OpenCL Libraries: ${OpenCL_LIBRARIES}")
