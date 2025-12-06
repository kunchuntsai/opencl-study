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

# ============================================================================
# Library: Core + Platform + Utils
# ============================================================================

# Option to build shared or static library
option(BUILD_SHARED_LIBS "Build shared library (.so/.dylib) instead of static (.a)" OFF)

# Collect library source files
file(GLOB_RECURSE CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/core/*.c")
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/*.c")
file(GLOB_RECURSE UTILS_SOURCES "${CMAKE_SOURCE_DIR}/src/utils/*.c")

# Create library (type determined by BUILD_SHARED_LIBS)
if(BUILD_SHARED_LIBS)
    add_library(opencl_imgproc SHARED
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
    # Version information for shared library
    set_target_properties(opencl_imgproc PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )
else()
    add_library(opencl_imgproc STATIC
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
endif()

# Library public include directories (for consumers like examples/)
target_include_directories(opencl_imgproc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include      # Public API: op_interface.h, op_registry.h, etc.
        ${OpenCL_INCLUDE_DIRS}           # OpenCL headers (needed by op_interface.h)
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src          # Internal: platform/, utils/, core/ (private)
)

# Link OpenCL to library
target_link_libraries(opencl_imgproc
    PUBLIC
        ${OpenCL_LIBRARIES}
    PRIVATE
        m  # Math library (internal use)
)

# Platform-specific settings for library
if(APPLE)
    target_compile_definitions(opencl_imgproc PUBLIC __APPLE__)
endif()

# ============================================================================
# User Algorithm Examples
# ============================================================================

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

# ============================================================================
# Main Executable
# ============================================================================

# Main application source
set(MAIN_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.c
)

# Create executable
add_executable(opencl_host
    ${MAIN_SOURCES}
    ${ALGO_SOURCES}          # User algorithms
)

# Link against library
target_link_libraries(opencl_host
    PRIVATE
        opencl_imgproc       # Framework library
)

# Main needs internal headers (platform/opencl_utils.h, utils/config.h)
target_include_directories(opencl_host
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src          # For main.c to access internal headers
)

# ============================================================================
# Configuration Summary
# ============================================================================

message(STATUS "=== OpenCL Image Processing Framework ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "OpenCL Libraries: ${OpenCL_LIBRARIES}")
message(STATUS "")
message(STATUS "Library Components:")
message(STATUS "  Core sources: ${CORE_SOURCES}")
message(STATUS "  Platform sources: ${PLATFORM_SOURCES}")
message(STATUS "  Utils sources: ${UTILS_SOURCES}")
message(STATUS "")
message(STATUS "Public API (include/):")
message(STATUS "  ${CMAKE_SOURCE_DIR}/include")
message(STATUS "")
message(STATUS "User Algorithms (examples/):")
message(STATUS "  ${ALGO_SOURCES}")
message(STATUS "")
message(STATUS "Output:")
message(STATUS "  Library: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libopencl_imgproc.a")
message(STATUS "  Executable: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/opencl_host")
