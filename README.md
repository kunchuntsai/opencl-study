# OpenCL Image Processing Framework

A modular, plugin-like C framework for developing and benchmarking OpenCL image processing algorithms with automatic verification and performance testing.

**What does this framework provide?**

- 🚀 **Rapid algorithm development**: Add new OpenCL algorithms with minimal boilerplate
- ✅ **Automatic correctness verification**: Compare GPU results against CPU reference implementation
- ⚡ **Performance benchmarking**: Built-in timing and speedup calculations
- 🔧 **Flexible configuration**: Per-algorithm `.ini` files with support for multiple kernel variants
- 💾 **Smart caching**: Automatic caching of compiled kernels and golden samples
- 🎯 **Clean architecture**: Modular design following best practices for safety-critical systems

## Quick Start

Get up and running in 3 steps:

```bash
# 1. Generate test image
python3 scripts/generate_test_image.py

# 2. Build the project
./scripts/build.sh

# 3. Run an algorithm
./build/opencl_host dilate3x3 0        # Morphological dilation
./build/opencl_host gaussian5x5 0      # Gaussian blur
```

**New to the framework?** See [Getting Started](#getting-started) below for detailed setup instructions.

## Table of Contents

- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Initial Setup](#initial-setup)
  - [Running Your First Algorithm](#running-your-first-algorithm)
- [Adding a New Algorithm](#adding-a-new-algorithm)
- [Configuration Guide](#configuration-guide)
- [Project Structure](#project-structure)
- [Documentation](#documentation)

## Getting Started

### Prerequisites

Before you begin, ensure you have:

- **C Compiler**: GCC or compatible C compiler
- **OpenCL Runtime**:
  - macOS: Apple OpenCL framework (pre-installed)
  - Linux/Windows: OpenCL SDK for your GPU vendor
- **Python 3**: With NumPy for test image generation
  ```bash
  pip install numpy
  ```

### Initial Setup

1. **Clone and navigate to the project:**
   ```bash
   cd opencl-study
   ```

2. **Generate a test image:**
   ```bash
   python3 scripts/generate_test_image.py
   ```
   This creates `test_data/input.bin` (1024x1024 grayscale image with geometric patterns).

3. **Build the project:**
   ```bash
   ./scripts/build.sh
   ```
   This compiles all sources and creates `build/opencl_host`.

### Running Your First Algorithm

Run an algorithm by name with a variant index:

```bash
./build/opencl_host dilate3x3 0
```

**What happens:**
- Loads configuration from `config/dilate3x3.ini`
- Runs C reference implementation (CPU)
- Runs OpenCL kernel variant 0 (GPU)
- Verifies GPU output matches CPU reference
- Displays performance metrics

**Try other algorithms:**
```bash
./build/opencl_host gaussian5x5 0      # Gaussian blur
./build/opencl_host dilate3x3 1        # Optimized dilate variant
./build/opencl_host --help             # See all available algorithms
```

## Adding a New Algorithm

Want to add your own image processing algorithm? It's straightforward!

**Quick start with automation:**

Use the `create_new_algo` script to generate all required files automatically:

```bash
# Interactive mode
./scripts/create_new_algo.sh

# Or with arguments
./scripts/create_new_algo.sh myalgo "My Algorithm Description"
```

This creates:
- Configuration file: `config/myalgo.ini`
- C reference: `src/myalgo/c_ref/myalgo_ref.c` (with 3 required functions)
- OpenCL kernel: `src/myalgo/cl/myalgo0.cl`
- Test data directory: `test_data/myalgo/`

Then build and run:
```bash
./scripts/build.sh
./build/opencl_host myalgo 0
```

**Manual approach (three simple steps):**

1. **Create a configuration file:** `config/myalgo.ini`
2. **Implement C reference:** `src/myalgo/c_ref/myalgo_ref.c` (3 required functions)
3. **Write OpenCL kernel:** `src/myalgo/cl/myalgo0.cl`

Then regenerate the registry and build:
```bash
./scripts/generate_registry.sh
./scripts/build.sh
./build/opencl_host myalgo 0
```

**📖 Detailed guide:** See **[docs/ADD_NEW_ALGO.md](docs/ADD_NEW_ALGO.md)** for complete step-by-step instructions, API requirements, and examples.

## Configuration Guide

Each algorithm has its own `.ini` configuration file in the `config/` directory.

**Basic structure:**
```ini
[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080

[kernel.v0]
kernel_file = src/myalgo/cl/myalgo0.cl
kernel_function = myalgo
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

**Key points:**
- Algorithm ID auto-derived from filename (`myalgo.ini` → `myalgo`)
- At least one kernel variant required (`[kernel.v0]`)
- Custom buffers supported via `[buffer.*]` sections

**📖 Complete reference:** See **[docs/CONFIG_SYSTEM.md](docs/CONFIG_SYSTEM.md)** for all configuration parameters, custom buffers, and advanced options.

## Features

- **Modular Architecture**: Easy to add new algorithms following the plugin pattern
- **Auto-Registration System**: Algorithms register themselves automatically - no manual registration needed
- **Per-Algorithm Configs**: Each algorithm has its own configuration file for better maintainability
- **Multiple Kernel Variants**: Support for different implementations of the same algorithm
- **Automatic Verification**: Built-in C reference implementations for correctness checking
- **Golden Sample Caching**: Automatic caching of c_ref outputs for regression testing
- **Kernel Binary Caching**: Compiled OpenCL binaries are cached per algorithm to skip recompilation
- **Performance Benchmarking**: Automatic timing and speedup calculations
- **Flexible Parameters**: OpParams structure supports algorithms with varying requirements
- **Intuitive CLI**: Algorithm selection by name (`./opencl_host dilate3x3 0`)
- **Clean Architecture**: Separated init/build/run phases for OpenCL operations
- **Organized Cache Structure**: Per-algorithm cache organization under `test_data/`

## Project Structure

```
.
├── ARCHITECTURE.md                 # Detailed architecture documentation
├── README.md                       # This file
├── config/
│   ├── dilate3x3.ini               # Dilate algorithm config
│   └── gaussian5x5.ini             # Gaussian algorithm config
├── docs/
│   ├── ADD_NEW_ALGO.md             # Algorithm development guide
│   ├── CONFIG_SYSTEM.md            # Configuration system guide
│   └── Doxyfile                    # Doxygen documentation config
├── scripts/
│   ├── build.sh                    # Build script (with --clean option)
│   ├── create_new_algo.sh          # Generate new algorithm template
│   ├── generate_registry.sh        # Auto-generate algorithm registry
│   ├── generate_test_image.py      # Test image generator
│   ├── generate_gaussian_kernels.py # Generate Gaussian kernel weights
│   └── run.sh                      # Interactive run script
├── src/
│   ├── main.c                      # Entry point with CLI parsing
│   ├── Makefile                    # Build configuration
│   ├── algorithm_runner.c/.h       # Algorithm execution pipeline
│   ├── dilate/
│   │   ├── c_ref/
│   │   │   └── dilate3x3_ref.c     # CPU reference + verification
│   │   └── cl/
│   │       ├── dilate0.cl          # Basic variant
│   │       └── dilate1.cl          # Optimized with local memory
│   ├── gaussian/
│   │   ├── c_ref/
│   │   │   └── gaussian5x5_ref.c   # CPU reference + verification
│   │   └── cl/
│   │       ├── gaussian0.cl        # Basic variant
│   │       └── gaussian1.cl        # Optimized variant
│   └── utils/
│       ├── auto_registry.c         # Auto-generated registry (don't edit)
│       ├── cache_manager.c/.h      # Kernel binary & golden caching
│       ├── cl_extension_api.c/.h   # Custom host API framework
│       ├── config.c/.h              # Configuration file parser
│       ├── image_io.c/.h           # Raw image I/O
│       ├── op_interface.h          # OpParams structure & API
│       ├── op_registry.c/.h        # Algorithm registration system
│       ├── opencl_utils.c/.h       # OpenCL utilities (init/build/run)
│       ├── safe_ops.h              # MISRA-C safe operations
│       └── verify.c/.h             # Verification utilities
└── test_data/
    ├── dilate3x3/
    │   ├── input.bin               # Algorithm-specific input
    │   ├── output.bin              # Latest output
    │   ├── golden/                 # Cached CPU reference output
    │   └── kernels/                # Cached compiled kernels (.bin)
    ├── gaussian5x5/
    │   ├── input.bin               # Algorithm-specific input
    │   ├── output.bin              # Latest output
    │   ├── kernel_x.bin            # Horizontal kernel weights
    │   ├── kernel_y.bin            # Vertical kernel weights
    │   ├── golden/                 # Cached CPU reference output
    │   └── kernels/                # Cached compiled kernels (.bin)
    └── output.bin                  # Global output (from root input)
```

**Note:** Build artifacts (`build/`) are created during compilation and can be cleaned with `./scripts/build.sh --clean`.

## Advanced Usage

### Build Options

```bash
./scripts/build.sh                # Incremental build
./scripts/build.sh --clean        # Clean build (removes build/ and cache/)
./scripts/build.sh --help         # Show help
```

Or build manually:
```bash
cd src && make                    # Incremental build
cd src && make clean && make      # Clean build (removes build/ only)
```

### Running Algorithms

**By algorithm name (recommended):**
```bash
./build/opencl_host <algorithm> [variant]
./build/opencl_host dilate3x3 0      # Run variant v0
./build/opencl_host gaussian5x5 0    # Run variant v0
```

**Interactive variant selection:**
```bash
./build/opencl_host dilate3x3        # Prompts for variant
```

**With explicit config path:**
```bash
./build/opencl_host config/dilate3x3.ini 0
```

**Using run script:**
```bash
./scripts/run.sh                     # Interactive mode
```

**Available algorithms:**
- `dilate3x3` - Morphological dilation (variants: v0, v1)
- `gaussian5x5` - Gaussian blur (variants: v0, v1, v2)

### Caching System

The framework automatically caches:
- **Kernel Binaries**: `test_data/{algorithm}/kernels/*.bin` (avoids recompilation)
- **Golden Samples**: `test_data/{algorithm}/golden/*.bin` (for regression testing)

Clear all caches with `./scripts/build.sh --clean`.

### Image Format

Images use raw grayscale format (8-bit unsigned, row-major).

**Convert to PNG for viewing:**
```bash
convert -depth 8 -size 1024x1024 gray:test_data/output.bin output.png
```

## Documentation

### 📚 Essential Guides

Start here if you're new to the framework:

- **[docs/ADD_NEW_ALGO.md](docs/ADD_NEW_ALGO.md)** - Complete guide to adding new algorithms
  - Three required components (config, C reference, OpenCL kernel)
  - Mandatory API functions
  - Auto-registration system
  - Step-by-step examples

- **[docs/CONFIG_SYSTEM.md](docs/CONFIG_SYSTEM.md)** - Configuration system reference
  - Per-algorithm `.ini` file format
  - Configuration parameters
  - Custom buffers
  - Multiple kernel variants

### 🏗️ Architecture & Design

Deep dive into the framework architecture:

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Detailed architecture and design decisions
