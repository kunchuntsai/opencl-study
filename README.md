# OpenCL Image Processing Framework

A modular, plugin-like C implementation for OpenCL image processing algorithms with built-in verification and benchmarking.

## Features

- **Modular Architecture**: Easy to add new algorithms following the plugin pattern
- **Multiple Kernel Variants**: Support for different implementations of the same algorithm
- **Automatic Verification**: Built-in C reference implementations for correctness checking
- **Golden Sample Caching**: Automatic caching of c_ref outputs for regression testing
- **Kernel Binary Caching**: Compiled OpenCL binaries are cached per algorithm to skip recompilation
- **Performance Benchmarking**: Automatic timing and speedup calculations
- **Configuration-Driven**: Runtime configuration via INI file without recompilation
- **MISRA-C:2023 Compliant**: High compliance for safety-critical applications
- **Clean Architecture**: Separated init/build/run phases for OpenCL operations
- **Organized Cache Structure**: Per-algorithm cache organization under `test_data/`

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
│       ├── opencl_utils.c/.h       # OpenCL init/build/run utilities
│       ├── config_parser.c/.h      # INI file parser
│       ├── image_io.c/.h           # Raw image I/O
│       └── safe_ops.h              # MISRA-C safe arithmetic operations
├── scripts/
│   ├── build.sh                    # Build script (with --clean option)
│   ├── generate_test_image.py     # Test image generator
│   └── run.sh                      # Run script
└── test_data/
    ├── dilate3x3/                  # Algorithm-specific cache
    │   ├── kernels/                # Cached kernel binaries (.bin)
    │   └── golden/                 # Golden sample from c_ref (.bin)
    ├── gaussian5x5/                # Algorithm-specific cache
    │   ├── kernels/                # Cached kernel binaries (.bin)
    │   └── golden/                 # Golden sample from c_ref (.bin)
    ├── input.bin                   # Sample input image
    └── output.bin                  # Latest output image
```

## Building

First, generate the test image:
```bash
python3 scripts/generate_test_image.py
```

Then build using the build script:
```bash
./scripts/build.sh                # Incremental build
./scripts/build.sh --clean        # Clean build (removes build/ and cache/)
./scripts/build.sh --help         # Show help
```

Or manually:
```bash
cd src
make                              # Incremental build
make clean && make                # Clean build (removes build/ only)
```

This will compile all source files and create the `opencl_host` executable in the `build/` directory.

### Caching System

The framework automatically caches:
- **Kernel Binaries**: Compiled OpenCL binaries in `test_data/{algorithm}/kernels/` (avoids recompilation)
- **Golden Samples**: C reference outputs in `test_data/{algorithm}/golden/` (for regression testing)

Use `./scripts/build.sh --clean` to remove all cached data and perform a fresh build.

## Running

### Interactive Mode (with prompts)

Using the run script without arguments:
```bash
./scripts/run.sh
```

This will prompt you to select an algorithm and variant interactively.

### Command-Line Mode (direct execution)

Specify algorithm and variant indices:
```bash
./scripts/run.sh <algorithm_index> <variant_index>

# Examples:
./scripts/run.sh 0 0    # Run dilate3x3 with variant v0
./scripts/run.sh 0 1    # Run dilate3x3 with variant v1
./scripts/run.sh 1 0    # Run gaussian5x5 with variant v0
```

Or run directly:
```bash
./build/opencl_host <algorithm_index> <variant_index>
```

**Algorithm Indices:**
- `0` = dilate3x3
- `1` = gaussian5x5

**Variant Indices:**
- `0` = v0 (basic)
- `1` = v1 (optimized, if available)

Note: The executable runs from the project root directory, so all paths in `config/config.ini` are relative to the project root.

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

### 1. Create Directory Structure
```bash
mkdir -p src/erode/c_ref
mkdir -p src/erode/cl
mkdir -p test_data/erode
```

### 2. Implement C Reference (for verification)
**File:** `src/erode/c_ref/erode3x3_ref.h`
```c
#pragma once

void erode3x3_ref(unsigned char* input, unsigned char* output,
                  int width, int height);
```

**File:** `src/erode/c_ref/erode3x3_ref.c`
```c
#include "erode3x3_ref.h"

void erode3x3_ref(unsigned char* input, unsigned char* output,
                  int width, int height) {
    /* Implement your algorithm here */
    /* This serves as the ground truth for verification */
}
```

### 3. Create Algorithm Wrapper
**File:** `src/erode/erode3x3.h`
```c
#pragma once

#include "../utils/op_interface.h"

extern Algorithm erode3x3_algorithm;
```

**File:** `src/erode/erode3x3.c`
```c
#include "erode3x3.h"
#include "c_ref/erode3x3_ref.h"
#include <math.h>

/* Verification function */
static int erode3x3_verify(unsigned char* gpu_output,
                           unsigned char* ref_output,
                           int width, int height,
                           float* max_error) {
    /* Compare GPU output with reference */
    /* Return 1 if passed, 0 if failed */
}

/* Info function (optional) */
static void erode3x3_info(void) {
    /* Print algorithm information */
}

/* Algorithm definition */
Algorithm erode3x3_algorithm = {
    .name = "Erode 3x3",
    .id = "erode3x3",
    .reference_impl = erode3x3_ref,
    .verify_result = erode3x3_verify,
    .print_info = erode3x3_info
};
```

### 4. Create OpenCL Kernel(s)
**File:** `src/erode/cl/erode0.cl`
```c
__kernel void erode3x3(__global const uchar* input,
                       __global uchar* output,
                       int width,
                       int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    /* Implement your kernel here */
}
```

Add optimized variants as needed (`erode1.cl`, etc.)

### 5. Register Algorithm
**File:** `src/main.c`

Add include at the top:
```c
#include "erode/erode3x3.h"
```

Add to `register_all_algorithms()` function:
```c
static void register_all_algorithms(void) {
    Algorithm* const algorithms[] = {
        &dilate3x3_algorithm,
        &gaussian5x5_algorithm,
        &erode3x3_algorithm,     // Add your algorithm here
        NULL
    };

    int i = 0;
    while (algorithms[i] != NULL) {
        register_algorithm(algorithms[i]);
        i++;
    }
}
```

### 6. Update Configuration
**File:** `config/config.ini`

Add kernel configuration(s):
```ini
[kernel.erode_v0]
op_id = erode3x3
variant_id = v0
kernel_file = src/erode/cl/erode0.cl
kernel_function = erode3x3
work_dim = 2
global_work_size = 1024,1024
local_work_size = 16,16
```

### 7. Build and Test
```bash
cd src && make clean && make
cd ..
./build/opencl_host 2 0    # Algorithm index 2 (new), variant 0
```

### Notes
- **Makefile:** Automatically discovers `.c` files - no need to manually update
- **Index:** New algorithm will be index 2 (after dilate=0, gaussian=1)
- **MISRA-C:** Follow existing patterns for compliance:
  - Use `#pragma once` in headers
  - Check all return values
  - Use safe arithmetic from `safe_ops.h`
  - Add NULL parameter checks
  - Use const for read-only parameters

## Output

After running an algorithm, you'll see:

```
Parsed config file: config/config.ini (3 kernel configurations)

=== OpenCL Initialization ===
Using GPU device
Device: Apple M4 Pro
OpenCL initialized successfully
Registered algorithm: Dilate 3x3 (ID: dilate3x3)
Registered algorithm: Gaussian 5x5 (ID: gaussian5x5)

=== Running Dilate 3x3 (variant: v0) ===
Read image from test_data/input.bin (1024 x 1024)

=== C Reference Implementation ===
Reference time: 2.505 ms

=== Golden Sample Verification ===
No golden sample found, creating from C reference output...
Golden sample saved (1048576 bytes): test_data/dilate3x3/golden/dilate3x3.bin
Golden sample created successfully

=== Building OpenCL Kernel ===
Kernel compiled successfully
Kernel binary cached (1370 bytes): test_data/dilate3x3/kernels/dilate0.bin
Built kernel 'dilate3x3' from src/dilate/cl/dilate0.cl (cached as dilate0)

=== Running OpenCL Kernel ===
GPU kernel time: 0.002 ms

=== Results ===
C Reference time: 2.505 ms
OpenCL GPU time:  0.002 ms
Speedup:          1010.49x
Verification:     PASSED
Max error:        0.00
Wrote image to test_data/output.bin (1024 x 1024)
Output saved to: test_data/output.bin
OpenCL cleaned up
```

On subsequent runs with cached data:
```
=== Golden Sample Verification ===
Golden sample found, verifying c_ref output...
Loaded golden sample (1048576 bytes): test_data/dilate3x3/golden/dilate3x3.bin
✓ Verification PASSED: Output matches golden sample exactly

=== Building OpenCL Kernel ===
Found cached kernel binary for dilate0, loading...
Loaded cached kernel binary (1370 bytes): test_data/dilate3x3/kernels/dilate0.bin
Using cached kernel binary: dilate0
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

To clean build artifacts and cache:
```bash
./scripts/build.sh --clean        # Removes build/ and all caches (test_data/*/kernels/, test_data/*/golden/)
```

To clean only build artifacts:
```bash
cd src
make clean                         # Removes build/ only (keeps caches)
```

## Requirements

- GCC or compatible C compiler
- OpenCL runtime (Apple OpenCL framework on macOS, or OpenCL SDK on Linux/Windows)
- Python 3 with NumPy (for test image generation)

## Documentation

- **[IMPL_PLAN.md](IMPL_PLAN.md)** - Detailed architecture and design decisions
- **[MISRA_C_2023_COMPLIANCE.md](MISRA_C_2023_COMPLIANCE.md)** - MISRA-C:2023 compliance documentation
- **[CACHING_FEATURE.md](CACHING_FEATURE.md)** - Kernel binary and golden sample caching implementation
- **[STUDY.md](STUDY.md)** - Comprehensive codebase study and module documentation
