#!/bin/bash

# Build script for OpenCL Image Processing Framework
# Usage:
#   ./scripts/build.sh          - Build the project (incremental)
#   ./scripts/build.sh --clean  - Clean all build artifacts and cache, then build
#   ./scripts/build.sh --help   - Show this help message

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
OUT_DIR="$PROJECT_ROOT/out"
SRC_DIR="$PROJECT_ROOT/src"

show_help() {
    echo "OpenCL Image Processing Framework - Build Script (CMake)"
    echo ""
    echo "Usage:"
    echo "  ./scripts/build.sh          - Build the project (incremental)"
    echo "  ./scripts/build.sh --clean  - Clean all build artifacts and cache, then build"
    echo "  ./scripts/build.sh --help   - Show this help message"
    echo ""
    echo "Build artifacts:"
    echo "  Executable: $BUILD_DIR/opencl_host"
    echo "  CMake:      $BUILD_DIR/"
    echo "  Output:     $OUT_DIR/{algorithm}_{variant}_{timestamp}/"
}

clean_all() {
    echo "=== Cleaning All Build Artifacts and Output ==="

    if [ -d "$BUILD_DIR" ]; then
        echo "Removing build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi

    if [ -d "$OUT_DIR" ]; then
        echo "Removing output directory: $OUT_DIR"
        rm -rf "$OUT_DIR"
    fi

    # Note: test_data/ only contains input test data

    echo "Clean complete"
    echo ""
}

build_project() {
    # Generate auto-registry before building
    "$SCRIPT_DIR/generate_registry.sh"
    if [ $? -ne 0 ]; then
        echo "Failed to generate algorithm registry!"
        exit 1
    fi
    echo ""

    echo "=== Building OpenCL Image Processing Framework (CMake) ==="

    # Create build directory if it doesn't exist
    mkdir -p "$BUILD_DIR"

    # Configure with CMake (only if not already configured or if clean build)
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        echo "Configuring CMake..."
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release "$PROJECT_ROOT"
        if [ $? -ne 0 ]; then
            echo "CMake configuration failed!"
            exit 1
        fi
    fi

    # Build with CMake
    echo "Building..."
    cmake --build "$BUILD_DIR" --config Release

    if [ $? -eq 0 ]; then
        echo ""
        echo "Build successful!"
        echo "Executable: $BUILD_DIR/opencl_host"
    else
        echo "Build failed!"
        exit 1
    fi
}

# Main script logic
case "${1:-}" in
    --clean)
        clean_all
        build_project
        ;;
    --help|-h)
        show_help
        ;;
    "")
        build_project
        ;;
    *)
        echo "Error: Unknown option '$1'"
        echo ""
        show_help
        exit 1
        ;;
esac
