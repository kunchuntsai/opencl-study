cmake_minimum_required(VERSION 3.10)
project(OpenCL_Image_Proc LANGUAGES C CXX)

# Define the Project Root (one level up from /src)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ============================================================================
# Find OpenCL (Crucial Fix)
# ============================================================================
if(APPLE)
    # On macOS, OpenCL is a framework
    find_library(OPENCL_LIBRARIES OpenCL REQUIRED)
    set(OpenCL_LIBRARIES ${OPENCL_LIBRARIES})
    message(STATUS "Found OpenCL Framework: ${OpenCL_LIBRARIES}")
else()
    # On Linux/Android
    find_package(OpenCL REQUIRED)
endif()

# ============================================================================
# Source Files
# ============================================================================

file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/core/*.c")
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.c")
file(GLOB_RECURSE UTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c")
file(GLOB CJSON_SOURCES "${PROJECT_ROOT}/third_party/cjson/*.c")

file(GLOB ALGO_DIRS LIST_DIRECTORIES true "${PROJECT_ROOT}/examples/*")
set(ALGO_SOURCES "")
foreach(DIR ${ALGO_DIRS})
    if(IS_DIRECTORY ${DIR} AND EXISTS "${DIR}/c_ref")
        file(GLOB_RECURSE DIR_SOURCES "${DIR}/c_ref/*.c")
        list(APPEND ALGO_SOURCES ${DIR_SOURCES})
    endif()
endforeach()

# ============================================================================
# Main Executable
# ============================================================================

add_executable(opencl_host
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILS_SOURCES}
    ${CJSON_SOURCES}
    ${ALGO_SOURCES}
)

target_include_directories(opencl_host
    PRIVATE
        "${PROJECT_ROOT}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_ROOT}/third_party/cjson"
        ${OpenCL_INCLUDE_DIRS}
)

target_link_libraries(opencl_host
    PRIVATE
        ${OpenCL_LIBRARIES}  # This now contains the macOS framework or Linux .so
        m
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(opencl_host PRIVATE __APPLE__)
endif()

if(BUILD_ANDROID)
    target_compile_definitions(opencl_host PRIVATE BUILD_ANDROID)
endif()

target_compile_definitions(opencl_host PRIVATE
    CL_INCLUDE_DIR="${PROJECT_ROOT}/include/cl"
)

# ============================================================================
# Copy OpenCL kernel headers
# ============================================================================

add_custom_target(copy_cl_headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/cl"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_ROOT}/include/cl"
        "${CMAKE_BINARY_DIR}/cl"
    COMMENT "Copying OpenCL kernel headers to build directory"
)
add_dependencies(opencl_host copy_cl_headers)

# ============================================================================
# Summary
# ============================================================================
message(STATUS "OpenCL Libraries: ${OpenCL_LIBRARIES}")