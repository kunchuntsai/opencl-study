cmake_minimum_required(VERSION 3.10)
project(opencl-image-processing VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")

# Build type defaults to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find OpenCL
if(APPLE)
    # On macOS, use the system OpenCL framework
    find_library(OpenCL_LIBRARY OpenCL REQUIRED)
    set(OpenCL_LIBRARIES ${OpenCL_LIBRARY})
    set(OpenCL_FOUND TRUE)
else()
    # On Linux/Windows, use find_package
    find_package(OpenCL REQUIRED)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCL_INCLUDE_DIRS}
)

# Collect source files
file(GLOB_RECURSE CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/core/*.c")
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/*.c")
file(GLOB_RECURSE UTILS_SOURCES "${CMAKE_SOURCE_DIR}/src/utils/*.c")

# Find all algorithm directories in examples/ (those containing c_ref subdirectories)
file(GLOB ALGO_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/examples/*")
set(ALGO_SOURCES "")
foreach(DIR ${ALGO_DIRS})
    if(IS_DIRECTORY ${DIR})
        # Check if c_ref directory exists
        if(EXISTS "${DIR}/c_ref")
            file(GLOB_RECURSE DIR_SOURCES "${DIR}/c_ref/*.c")
            list(APPEND ALGO_SOURCES ${DIR_SOURCES})
        endif()
    endif()
endforeach()

# Main sources
set(MAIN_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.c
)

# Create executable
add_executable(opencl_host
    ${MAIN_SOURCES}
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILS_SOURCES}
    ${ALGO_SOURCES}
)

# Link libraries
target_link_libraries(opencl_host
    ${OpenCL_LIBRARIES}
    m  # Math library
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(opencl_host PRIVATE __APPLE__)
endif()

# Print configuration summary
message(STATUS "=== OpenCL Image Processing Framework ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "OpenCL Libraries: ${OpenCL_LIBRARIES}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Algorithm sources found: ${ALGO_SOURCES}")
