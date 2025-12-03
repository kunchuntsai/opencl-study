#!/bin/bash

# Build script for OpenCL Image Processing Framework
# Usage:
#   ./scripts/build.sh          - Build the project (incremental)
#   ./scripts/build.sh --clean  - Clean all build artifacts and cache, then build
#   ./scripts/build.sh --help   - Show this help message

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/out"
SRC_DIR="$PROJECT_ROOT/src"

show_help() {
    echo "OpenCL Image Processing Framework - Build Script"
    echo ""
    echo "Usage:"
    echo "  ./scripts/build.sh          - Build the project (incremental)"
    echo "  ./scripts/build.sh --clean  - Clean all build artifacts and cache, then build"
    echo "  ./scripts/build.sh --help   - Show this help message"
    echo ""
    echo "Build artifacts:"
    echo "  Executable: $BUILD_DIR/opencl_host"
    echo "  Objects:    $BUILD_DIR/obj/"
    echo "  Output:     out/{algorithm}_{variant}_{timestamp}/"
}

clean_all() {
    echo "=== Cleaning All Build Artifacts and Output ==="

    if [ -d "$BUILD_DIR" ]; then
        echo "Removing build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi

    # Note: Output directories with timestamps are now in out/
    # test_data/ only contains input test data

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

    echo "=== Building OpenCL Image Processing Framework ==="

    cd "$SRC_DIR"
    make

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
