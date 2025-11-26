# OpenCL Image Processing Framework

A modular, plugin-like C implementation for OpenCL image processing algorithms with built-in verification and benchmarking.

> **✨ Recent Updates:**
> - **Per-Algorithm Configs**: Each algorithm now has its own `.ini` file (e.g., `config/dilate3x3.ini`)
> - **Auto-Registration**: New `REGISTER_ALGORITHM()` macro eliminates boilerplate
> - **Flexible Parameters**: `OpParams` structure supports diverse algorithm requirements
> - **Intuitive CLI**: Run algorithms by name: `./opencl_host dilate3x3 0`

## Quick Start

```bash
# 1. Generate test image
python3 scripts/generate_test_image.py

# 2. Build the project
./scripts/build.sh

# 3. Run an algorithm by name
./build/opencl_host dilate3x3 0        # Morphological dilation
./build/opencl_host gaussian5x5 0      # Gaussian blur

# 4. See available options
./build/opencl_host --help
```

Each algorithm automatically loads its own configuration from `config/<algorithm>.ini`.

## Features

- **Modular Architecture**: Easy to add new algorithms following the plugin pattern
- **Auto-Registration System**: Algorithms register themselves using simple macros - no manual registration needed
- **Per-Algorithm Configs**: Each algorithm has its own configuration file for better maintainability
- **Multiple Kernel Variants**: Support for different implementations of the same algorithm
- **Automatic Verification**: Built-in C reference implementations for correctness checking
- **Golden Sample Caching**: Automatic caching of c_ref outputs for regression testing
- **Kernel Binary Caching**: Compiled OpenCL binaries are cached per algorithm to skip recompilation
- **Performance Benchmarking**: Automatic timing and speedup calculations
- **Flexible Parameters**: OpParams structure supports algorithms with varying requirements
- **Intuitive CLI**: Algorithm selection by name (`./opencl_host dilate3x3 0`)
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
│   ├── dilate3x3.ini               # Dilate algorithm config
│   ├── gaussian5x5.ini             # Gaussian algorithm config
│   └── README.md                   # Config documentation
├── docs/
│   ├── ADD_NEW_ALGO.md             # Algorithm development guide
│   ├── ALGORITHM_INTERFACE.md      # Algorithm interface guide
│   └── CONFIG_SYSTEM.md            # Per-algorithm config system guide
├── src/
│   ├── main.c                      # Entry point with smart CLI parsing
│   ├── Makefile                    # Build configuration
│   ├── dilate/
│   │   ├── c_ref/                  # C reference implementation
│   │   │   └── dilate3x3_ref.c     # Self-contained, auto-registers
│   │   └── cl/                     # OpenCL kernels
│   │       ├── dilate0.cl          # Basic variant
│   │       └── dilate1.cl          # Optimized with local memory
│   ├── gaussian/
│   │   ├── c_ref/                  # C reference implementation
│   │   │   └── gaussian5x5_ref.c   # Self-contained, auto-registers
│   │   └── cl/                     # OpenCL kernels
│   │       └── gaussian0.cl        # Basic variant
│   └── utils/
│       ├── op_interface.h          # OpParams + Algorithm interface
│       ├── op_registry.c/.h        # Registration + REGISTER_ALGORITHM macro
│       ├── opencl_utils.c/.h       # OpenCL init/build/run utilities
│       ├── config_parser.c/.h      # INI file parser
│       ├── image_io.c/.h           # Raw image I/O
│       ├── verify.c/.h             # Verification utilities
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

The framework supports multiple ways to run algorithms:

### By Algorithm Name (Recommended)
```bash
./build/opencl_host <algorithm> [variant]

# Examples:
./build/opencl_host dilate3x3 0      # Run dilate with variant v0
./build/opencl_host gaussian5x5 0    # Run gaussian with variant v0
./build/opencl_host dilate3x3 1      # Run dilate with optimized variant v1
```

This automatically loads `config/<algorithm>.ini` for the specified algorithm.

### Interactive Variant Selection
```bash
./build/opencl_host dilate3x3        # Prompts for variant selection
./build/opencl_host gaussian5x5      # Prompts for variant selection
```

### With Explicit Config Path
```bash
./build/opencl_host config/dilate3x3.ini 0
./build/opencl_host config/custom.ini 1
```

### Using the Run Script
```bash
./scripts/run.sh                     # Interactive mode
```

### Help
```bash
./build/opencl_host --help           # Show usage and available algorithms
```

**Available Algorithms:**
- `dilate3x3` - Morphological dilation (variants: v0, v1)
- `gaussian5x5` - Gaussian blur (variants: v0)

**Variant Indices:**
- `0` = v0 (basic implementation)
- `1` = v1 (optimized, if available)

Note: The executable runs from the project root directory, so all paths in config files are relative to the project root.

## Configuration

Each algorithm has its own configuration file: `config/<algorithm>.ini`

**Key Features:**
- One config file per algorithm for better organization
- Algorithm ID auto-derived from filename (`dilate3x3.ini` → `op_id = dilate3x3`)
- Multiple kernel variants per algorithm
- No merge conflicts when adding new algorithms

**Example:** `config/dilate3x3.ini`
```ini
[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080

[kernel.v0]
kernel_file = src/dilate/cl/dilate0.cl
kernel_function = dilate3x3
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

For detailed configuration options and examples, see **[docs/CONFIG_SYSTEM.md](docs/CONFIG_SYSTEM.md)** and **[config/README.md](config/README.md)**.

## Algorithms

The framework includes morphological and filtering algorithms. For the complete list of implemented algorithms and detailed instructions on adding new ones, see **[docs/ADD_NEW_ALGO.md](docs/ADD_NEW_ALGO.md)**.

**Quick summary:**
- **Dilate 3x3** - Morphological dilation (variants: v0, v1)
- **Gaussian 5x5** - Gaussian blur (variants: v0)

**Adding a new algorithm is simple:**
1. Create `.c` file with `REGISTER_ALGORITHM()` macro
2. Create OpenCL `.cl` kernel
3. Create `config/<algorithm>.ini`
4. Build and run!

See the full guide: **[docs/ADD_NEW_ALGO.md](docs/ADD_NEW_ALGO.md)**

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

### Architecture & Design
- **[STUDY.md](STUDY.md)** - Comprehensive codebase study and module documentation
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Detailed architecture and design decisions

### Feature Guides
- **[docs/ADD_NEW_ALGO.md](docs/ADD_NEW_ALGO.md)** - Algorithm development guide (NEW)
- **[docs/CONFIG_SYSTEM.md](docs/CONFIG_SYSTEM.md)** - Per-algorithm configuration system (NEW)
- **[docs/ALGORITHM_INTERFACE.md](docs/ALGORITHM_INTERFACE.md)** - Flexible algorithm interface guide (NEW)
- **[CACHING_FEATURE.md](CACHING_FEATURE.md)** - Kernel binary and golden sample caching

### Compliance & Standards
- **[MISRA_C_2023_COMPLIANCE.md](MISRA_C_2023_COMPLIANCE.md)** - MISRA-C:2023 compliance documentation

### Quick References
- **[config/README.md](config/README.md)** - Configuration file format reference
