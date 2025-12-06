#!/bin/bash

# SDK Packaging Script for OpenCL Image Processing Framework
# Creates a distributable SDK package with:
# - Compiled library (.so or .a)
# - Public headers
# - Example main.c
# - Documentation

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# SDK configuration
VERSION="1.0.0"
SDK_NAME="opencl-imgproc-sdk"
SDK_DIR="$PROJECT_ROOT/${SDK_NAME}-${VERSION}"
OUTPUT_TARBALL="$PROJECT_ROOT/${SDK_NAME}-${VERSION}.tar.gz"

show_help() {
    echo "OpenCL Image Processing Framework - SDK Packaging Script"
    echo ""
    echo "Usage:"
    echo "  ./scripts/create_sdk.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --shared          Build shared library (.so) [default]"
    echo "  --static          Build static library (.a)"
    echo "  --with-source     Include full source code in SDK"
    echo "  --help            Show this help message"
    echo ""
    echo "Output:"
    echo "  Creates: ${SDK_NAME}-${VERSION}.tar.gz"
    echo ""
    echo "SDK Contents:"
    echo "  lib/              Compiled library"
    echo "  include/          Public API headers"
    echo "  examples/         Sample code (main.c + algorithms)"
    echo "  docs/             Documentation"
}

# Parse arguments
BUILD_SHARED=true
WITH_SOURCE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --shared)
            BUILD_SHARED=true
            shift
            ;;
        --static)
            BUILD_SHARED=false
            shift
            ;;
        --with-source)
            WITH_SOURCE=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            echo "Error: Unknown option '$1'"
            show_help
            exit 1
            ;;
    esac
done

echo "=== Creating SDK Package ==="
echo ""
echo "Configuration:"
echo "  Version:        ${VERSION}"
echo "  Library Type:   $([ "$BUILD_SHARED" = true ] && echo "Shared (.so)" || echo "Static (.a)")"
echo "  Include Source: $([ "$WITH_SOURCE" = true ] && echo "Yes" || echo "No")"
echo ""

# Clean previous SDK builds
if [ -d "$SDK_DIR" ]; then
    echo "Cleaning previous SDK directory..."
    rm -rf "$SDK_DIR"
fi

if [ -f "$OUTPUT_TARBALL" ]; then
    echo "Removing previous tarball..."
    rm -f "$OUTPUT_TARBALL"
fi

# Build the library
echo ""
echo "=== Building Library ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ "$BUILD_SHARED" = true ]; then
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=ON \
          "$PROJECT_ROOT"
else
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=OFF \
          "$PROJECT_ROOT"
fi

cmake --build . --config Release

echo ""
echo "=== Creating SDK Directory Structure ==="

mkdir -p "$SDK_DIR"/{lib,include,examples,docs}

# Copy compiled library
echo "Copying library..."
if [ "$BUILD_SHARED" = true ]; then
    if [ -f "$BUILD_DIR/lib/libopencl_imgproc.dylib" ]; then
        cp "$BUILD_DIR/lib/libopencl_imgproc.dylib" "$SDK_DIR/lib/"
    elif [ -f "$BUILD_DIR/lib/libopencl_imgproc.so" ]; then
        cp "$BUILD_DIR/lib/libopencl_imgproc.so" "$SDK_DIR/lib/"
    fi
else
    cp "$BUILD_DIR/lib/libopencl_imgproc.a" "$SDK_DIR/lib/"
fi

# Copy public headers
echo "Copying public headers..."
cp -r "$PROJECT_ROOT/include"/* "$SDK_DIR/include/"

# Copy example main.c
echo "Copying example code..."
cp "$PROJECT_ROOT/src/main.c" "$SDK_DIR/examples/main.c.example"

# Copy sample algorithms
cp -r "$PROJECT_ROOT/examples"/* "$SDK_DIR/examples/"

# Copy configuration examples
if [ -d "$PROJECT_ROOT/config" ]; then
    cp -r "$PROJECT_ROOT/config" "$SDK_DIR/examples/"
fi

# Copy documentation
echo "Copying documentation..."
if [ -f "$PROJECT_ROOT/README.md" ]; then
    cp "$PROJECT_ROOT/README.md" "$SDK_DIR/docs/"
fi

if [ -f "$PROJECT_ROOT/docs/CLEAN_ARCHITECTURE_ANALYSIS.md" ]; then
    cp "$PROJECT_ROOT/docs/CLEAN_ARCHITECTURE_ANALYSIS.md" "$SDK_DIR/docs/"
fi

if [ -f "$PROJECT_ROOT/docs/SDK_PACKAGING.md" ]; then
    cp "$PROJECT_ROOT/docs/SDK_PACKAGING.md" "$SDK_DIR/docs/"
fi

if [ -f "$PROJECT_ROOT/include/README.md" ]; then
    cp "$PROJECT_ROOT/include/README.md" "$SDK_DIR/docs/API_REFERENCE.md"
fi

# Optionally include full source
if [ "$WITH_SOURCE" = true ]; then
    echo "Including full source code..."
    mkdir -p "$SDK_DIR/src"
    cp -r "$PROJECT_ROOT/src"/* "$SDK_DIR/src/"
    cp "$PROJECT_ROOT/CMakeLists.txt" "$SDK_DIR/"
    cp -r "$PROJECT_ROOT/scripts" "$SDK_DIR/"
fi

# Create SDK README
cat > "$SDK_DIR/README.md" <<'EOF'
# OpenCL Image Processing Framework SDK

## Quick Start

### 1. Extract SDK

```bash
tar -xzf opencl-imgproc-sdk-*.tar.gz
cd opencl-imgproc-sdk-*/
```

### 2. Review Examples

- `examples/main.c.example` - Reference main implementation
- `examples/dilate/` - Dilation algorithm example
- `examples/gaussian/` - Gaussian blur example

### 3. Implement Your Algorithm

Create your algorithm in `my_algorithms/myalgo/c_ref/myalgo_ref.c`:

```c
#include "op_interface.h"
#include "op_registry.h"

void MyAlgoRef(const OpParams* params) {
    // Your CPU implementation
}

int MyAlgoVerify(const OpParams* params, float* max_error) {
    // Verification logic
}

int MyAlgoSetKernelArgs(cl_kernel kernel, cl_mem input_buf,
                        cl_mem output_buf, const OpParams* params) {
    // Set kernel arguments
}

REGISTER_ALGORITHM(myalgo, "My Algorithm", MyAlgoRef, MyAlgoVerify, MyAlgoSetKernelArgs)
```

### 4. Build Your Application

```bash
# Create your CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(MyApp)

include_directories(${CMAKE_SOURCE_DIR}/sdk/include)
link_directories(${CMAKE_SOURCE_DIR}/sdk/lib)

file(GLOB_RECURSE ALGO_SOURCES "my_algorithms/*/c_ref/*.c")

add_executable(my_app
    main.c
    ${ALGO_SOURCES}
)

target_link_libraries(my_app
    opencl_imgproc
    OpenCL
    m
)
```

```bash
mkdir build && cd build
cmake ..
make
```

### 5. Run

```bash
./my_app config/myalgo.ini 0
```

## Documentation

- `docs/API_REFERENCE.md` - Public API documentation
- `docs/SDK_PACKAGING.md` - SDK usage guide
- `docs/CLEAN_ARCHITECTURE_ANALYSIS.md` - Architecture overview

## Support

For issues, questions, or contributions, please visit:
https://github.com/kunchuntsai/opencl-study
EOF

# Create version file
cat > "$SDK_DIR/VERSION" <<EOF
OpenCL Image Processing Framework SDK
Version: ${VERSION}
Build Date: $(date '+%Y-%m-%d %H:%M:%S')
Library Type: $([ "$BUILD_SHARED" = true ] && echo "Shared" || echo "Static")
EOF

# Create tarball
echo ""
echo "=== Creating Tarball ==="
cd "$PROJECT_ROOT"
tar -czf "$OUTPUT_TARBALL" "$(basename "$SDK_DIR")"

# Calculate sizes
SDK_SIZE=$(du -sh "$SDK_DIR" | cut -f1)
TARBALL_SIZE=$(du -sh "$OUTPUT_TARBALL" | cut -f1)

echo ""
echo "=== SDK Package Created Successfully ==="
echo ""
echo "SDK Directory:  $SDK_DIR"
echo "SDK Size:       $SDK_SIZE"
echo ""
echo "Tarball:        $OUTPUT_TARBALL"
echo "Tarball Size:   $TARBALL_SIZE"
echo ""
echo "Contents:"
echo "  - lib/              $(ls -1 "$SDK_DIR/lib" | wc -l | tr -d ' ') file(s)"
echo "  - include/          $(find "$SDK_DIR/include" -name "*.h" | wc -l | tr -d ' ') header(s)"
echo "  - examples/         $(find "$SDK_DIR/examples" -name "*.c" | wc -l | tr -d ' ') example(s)"
echo "  - docs/             $(ls -1 "$SDK_DIR/docs" | wc -l | tr -d ' ') document(s)"

if [ "$WITH_SOURCE" = true ]; then
    echo "  - src/              $(find "$SDK_DIR/src" -name "*.c" -o -name "*.h" | wc -l | tr -d ' ') source file(s)"
fi

echo ""
echo "To distribute:"
echo "  scp $OUTPUT_TARBALL user@server:/path/to/destination"
echo ""
echo "Customer extracts and uses:"
echo "  tar -xzf $(basename "$OUTPUT_TARBALL")"
echo "  cd $(basename "$SDK_DIR")"
echo "  # Follow README.md"
echo ""
