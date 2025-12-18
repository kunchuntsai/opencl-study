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

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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
# Third-party Libraries
# ============================================================================

# cJSON library for JSON parsing
set(CJSON_SOURCES "${CMAKE_SOURCE_DIR}/third_party/cjson/cJSON.c")
set(CJSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/cjson")

# ============================================================================
# Source Files
# ============================================================================

# Collect all source files
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

# ============================================================================
# Main Executable
# ============================================================================

# Create executable with all sources
add_executable(opencl_host
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILS_SOURCES}
    ${CJSON_SOURCES}
    ${ALGO_SOURCES}
)

# Include directories
target_include_directories(opencl_host
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include      # Public API headers
        ${CMAKE_SOURCE_DIR}/src          # Internal headers
        ${CJSON_INCLUDE_DIR}             # cJSON headers
        ${OpenCL_INCLUDE_DIRS}           # OpenCL headers
)

# Link libraries
target_link_libraries(opencl_host
    PRIVATE
        ${OpenCL_LIBRARIES}
        m  # Math library
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(opencl_host PRIVATE __APPLE__)
endif()

# Bake absolute paths for OpenCL kernel include directories
target_compile_definitions(opencl_host PRIVATE
    CL_INCLUDE_DIR="${CMAKE_SOURCE_DIR}/include/cl"
)

# ============================================================================
# Copy OpenCL kernel headers to build directory (portable deployment)
# ============================================================================

# Copy include/cl/ to build/cl/ for self-contained deployment
file(GLOB CL_HEADERS "${CMAKE_SOURCE_DIR}/include/cl/*.h")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cl")
foreach(HEADER ${CL_HEADERS})
    get_filename_component(HEADER_NAME ${HEADER} NAME)
    configure_file(${HEADER} "${CMAKE_BINARY_DIR}/cl/${HEADER_NAME}" COPYONLY)
endforeach()

# Custom target to ensure headers are copied on rebuild
add_custom_target(copy_cl_headers ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/include/cl"
        "${CMAKE_BINARY_DIR}/cl"
    COMMENT "Copying OpenCL kernel headers to build directory"
)
add_dependencies(opencl_host copy_cl_headers)

# ============================================================================
# Configuration Summary
# ============================================================================

message(STATUS "=== OpenCL Image Processing Framework ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "OpenCL Libraries: ${OpenCL_LIBRARIES}")
message(STATUS "")
message(STATUS "Sources:")
message(STATUS "  Core: ${CORE_SOURCES}")
message(STATUS "  Platform: ${PLATFORM_SOURCES}")
message(STATUS "  Utils: ${UTILS_SOURCES}")
message(STATUS "  Algorithms: ${ALGO_SOURCES}")
message(STATUS "")
message(STATUS "Output:")
message(STATUS "  Executable: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/opencl_host")
