# Library Release Directory

This directory contains the compiled library for distribution and releases.

## Contents

After running `./scripts/build.sh --lib`, you'll find:

- `libopencl_imgproc.so` - Shared library (Linux)
- `libopencl_imgproc.dylib` - Shared library (macOS)
- `libopencl_imgproc.a` - Static library

## Usage

### Building the Library

```bash
# Build shared library (.so/.dylib)
./scripts/build.sh --lib
```

The library will be automatically copied here from `build/lib/` for easy distribution.

### For SDK Distribution

This directory is ready to be packaged for SDK releases:

```bash
# Create SDK package
./scripts/create_sdk.sh --shared

# The lib/ directory will be included in the SDK package
```

### For Customers

Customers link against the library in this directory:

```bash
# Example customer build
gcc main.c my_algorithms.c \
    -I./include \
    -L./lib -lopencl_imgproc \
    -framework OpenCL \
    -o my_app
```

## Library Contents

The library contains:
- **Core**: Algorithm runner, operation registry
- **Platform**: OpenCL utilities, cache management
- **Utils**: Configuration parser, image I/O, verification

The library does NOT contain:
- Example algorithms (those are in `examples/`)
- Main application (that's in `src/main.c`)

## Versioning

Library version is set in `CMakeLists.txt`:
- VERSION: 1.0.0
- SOVERSION: 1

For shared libraries, this creates:
- `libopencl_imgproc.so.1.0.0` (actual file)
- `libopencl_imgproc.so.1` → `libopencl_imgproc.so.1.0.0` (major version symlink)
- `libopencl_imgproc.so` → `libopencl_imgproc.so.1` (development symlink)
