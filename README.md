# OpenCL Image Processing Framework

A modular, plugin-like C implementation for OpenCL image processing algorithms with built-in verification and benchmarking.

## Features

- **Modular Architecture**: Easy to add new algorithms following the plugin pattern
- **Multiple Kernel Variants**: Support for different implementations of the same algorithm
- **Automatic Verification**: Built-in C reference implementations for correctness checking
- **Performance Benchmarking**: Automatic timing and speedup calculations
- **Configuration-Driven**: Runtime configuration via INI file without recompilation
- **Interactive Menu**: User-friendly console interface for algorithm selection

## Project Structure

```
.
├── build/
│   ├── obj/                        # Intermediate object files
│   └── opencl_host                 # Compiled executable
├── config/
│   └── config.ini                  # Runtime configuration
├── src/
│   ├── main.c                      # Entry point, menu system
│   ├── Makefile                    # Build configuration
│   ├── dilate/
│   │   ├── c_ref/                  # C reference implementation
│   │   │   ├── dilate3x3_ref.c
│   │   │   └── dilate3x3_ref.h
│   │   ├── cl/                     # OpenCL kernels
│   │   │   ├── dilate0.cl          # Basic variant
│   │   │   └── dilate1.cl          # Optimized with local memory
│   │   ├── dilate3x3.c             # Algorithm wrapper
│   │   └── dilate3x3.h
│   ├── gaussian/
│   │   ├── c_ref/                  # C reference implementation
│   │   │   ├── gaussian5x5_ref.c
│   │   │   └── gaussian5x5_ref.h
│   │   ├── cl/                     # OpenCL kernels
│   │   │   └── gaussian0.cl        # Basic variant
│   │   ├── gaussian5x5.c           # Algorithm wrapper
│   │   └── gaussian5x5.h
│   └── utils/
│       ├── op_interface.h          # Common interface for all algorithms
│       ├── op_registry.c/.h        # Algorithm registration and dispatch
│       ├── opencl_utils.c/.h       # OpenCL initialization and utilities
│       ├── config_parser.c/.h      # INI file parser
│       └── image_io.c/.h           # Raw image I/O
├── scripts/
│   ├── build.sh                    # Build script
│   └── run.sh                      # Run script
└── test_data/
    ├── dilate/                     # Test data for dilate
    ├── gaussian/                   # Test data for gaussian
    └── input.bin                   # Sample input image
```

## Building

First, generate the test image:
```bash
python3 scripts/generate_test_image.py
```

Then build using the build script:
```bash
./scripts/build.sh
```

Or manually:
```bash
cd src
make
```

This will compile all source files and create the `opencl_host` executable in the `build/` directory.

## Running

Using the run script:
```bash
./scripts/run.sh
```

Or manually:
```bash
./build/opencl_host
```

Note: The executable runs from the project root directory, so all paths in `config/config.ini` are relative to the project root.

The program will present an interactive menu:

```
=== OpenCL Image Processing Framework ===
1. Dilate 3x3
2. Gaussian 5x5
0. Exit
Select algorithm:
```

After selecting an algorithm, you can choose from available kernel variants:

```
Available variants for Dilate 3x3:
1. v0 (kernels/dilate0.cl - dilate3x3)
2. v1 (kernels/dilate1.cl - dilate3x3_optimized)
Select variant:
```

## Configuration

Edit `config/config.ini` to configure:
- Input/output image paths and dimensions
- Kernel file paths and function names
- Work group sizes (global and local)

Example configuration:

```ini
[image]
input = test_data/input.bin
output = test_data/output.bin
width = 1024
height = 1024

[kernel.dilate_v0]
op_id = dilate3x3
variant_id = v0
kernel_file = src/dilate/cl/dilate0.cl
kernel_function = dilate3x3
work_dim = 2
global_work_size = 1024,1024
local_work_size = 16,16
```

Note: All paths in the config file are relative to the project root directory.

## Implemented Algorithms

### 1. Dilate 3x3
Morphological dilation with a 3x3 structuring element.

**Variants:**
- `v0`: Basic implementation
- `v1`: Optimized with local memory tiling

### 2. Gaussian 5x5
Gaussian blur with a 5x5 kernel.

**Variants:**
- `v0`: Basic implementation

## Adding New Algorithms

To add a new algorithm (e.g., `erode3x3`):

1. Create a new directory: `src/erode/`
2. Create subdirectories: `src/erode/c_ref/` and `src/erode/cl/`
3. Implement C reference in `src/erode/c_ref/erode3x3_ref.c/.h`
4. Create algorithm wrapper in `src/erode/erode3x3.c/.h` (include `../utils/op_interface.h`)
5. Add OpenCL kernel variants in `src/erode/cl/erode0.cl`, `erode1.cl`, etc.
6. Register in `src/main.c`:
   - Add `#include "erode/erode3x3.h"`
   - Add `register_algorithm(&erode3x3_algorithm);`
7. Update `config/config.ini` with new kernel configurations
8. Update `src/Makefile` to include new source files in SRCS
9. Create test data directories: `test_data/erode/`

## Output

After running an algorithm, you'll see:

```
Running C reference implementation...
Running OpenCL kernel: dilate3x3...

=== Results ===
C Reference time: 125.340 ms
OpenCL GPU time:  3.245 ms
Speedup:          38.63x
Verification:     PASSED
Max error:        0.00
Output saved to: test_data/output.bin
```

## Testing

A test image generator script is included:

```bash
python3 scripts/generate_test_image.py
```

This creates a 1024x1024 grayscale test image (`test_data/input.bin`) with geometric patterns and noise.

## Image Format

Images are stored in raw grayscale format (8-bit unsigned integers, row-major order).

To convert to PNG for viewing:
```bash
# Using ImageMagick
convert -depth 8 -size 1024x1024 gray:test_data/output.bin test_data/output.png
```

## Cleaning

```bash
cd src
make clean
```

This removes the entire `build/` directory, including all intermediate object files and the executable.

## Requirements

- GCC or compatible C compiler
- OpenCL runtime (Apple OpenCL framework on macOS, or OpenCL SDK on Linux/Windows)
- Python 3 with NumPy (for test image generation)

## Implementation Details

See [IMPL_PLAN.md](IMPL_PLAN.md) for detailed architecture and design decisions.
