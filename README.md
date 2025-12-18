# OpenCL Image Processing Framework

A modular, plugin-like C framework for developing and benchmarking OpenCL image processing algorithms with automatic verification and performance testing.

**What does this framework provide?**

- 🚀 **Rapid algorithm development**: Add new OpenCL algorithms with minimal boilerplate
- ✅ **Automatic correctness verification**: Compare GPU results against CPU reference implementation
- ⚡ **Performance benchmarking**: Built-in timing and speedup calculations
- 🔧 **Flexible configuration**: Per-algorithm `.json` files with support for multiple kernel variants
- 💾 **Smart caching**: Automatic caching of compiled kernels and golden samples
- 🎯 **Clean architecture**: Modular design following Clean Architecture principles
- 📦 **SDK-ready**: Library packaging for distribution to customers

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
- **CMake**: Version 3.10 or higher
  - macOS: `brew install cmake`
  - Linux: `sudo apt-get install cmake` (Ubuntu/Debian)
  - Windows: Download from [cmake.org](https://cmake.org/download/)
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
- Loads configuration from `config/dilate3x3.json`
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
- Configuration file: `config/myalgo.json`
- C reference: `examples/myalgo/c_ref/myalgo_ref.c` (with 2 required functions)
- OpenCL kernel: `examples/myalgo/cl/myalgo0.cl`
- Test data directory: `test_data/myalgo/`

Then build and run:
```bash
./scripts/build.sh
./build/opencl_host myalgo 0
```

**Manual approach (three simple steps):**

1. **Create a configuration file:** `config/myalgo.json`
2. **Implement C reference:** `examples/myalgo/c_ref/myalgo_ref.c` (2 required functions)
3. **Write OpenCL kernel:** `examples/myalgo/cl/myalgo0.cl`

Then build (registry auto-generated):
```bash
./scripts/build.sh
./build/opencl_host myalgo 0
```

**📖 Detailed guide:** See **[docs/ADD_NEW_ALGO.md](docs/ADD_NEW_ALGO.md)** for complete step-by-step instructions, API requirements, and examples.

## Configuration Guide

Each algorithm has its own `.json` configuration file in the `config/` directory.

**Basic structure:**
```json
{
    "input": {
        "input_image_id": "image_1"
    },
    "output": {
        "output_image_id": "output_1"
    },
    "verification": {
        "tolerance": 0,
        "error_rate_threshold": 0,
        "golden_source": "c_ref"
    },
    "kernels": {
        "v0": {
            "kernel_file": "examples/myalgo/cl/myalgo0.cl",
            "kernel_function": "myalgo",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]}
            ]
        }
    }
}
```

**Key points:**
- Algorithm ID auto-derived from filename (`myalgo.json` → `myalgo`)
- At least one kernel variant required (`kernels.v0`)
- Custom buffers supported via `buffers` section
- Kernel arguments fully config-driven via `kernel_args`

**📖 Complete reference:** See **[docs/CONFIG_SYSTEM.md](docs/CONFIG_SYSTEM.md)** for all configuration parameters, custom buffers, and advanced options.

## Features

- **Modular Architecture**: Easy to add new algorithms following the plugin pattern
- **Clean Architecture**: Compliant with Clean Architecture principles (Dependency Inversion, Stable Abstractions)
- **SDK Distribution**: Package core functionality as library, distribute to customers
- **Auto-Registration System**: Algorithms register themselves automatically - no manual registration needed
- **Per-Algorithm Configs**: Each algorithm has its own configuration file for better maintainability
- **Multiple Kernel Variants**: Support for different implementations of the same algorithm
- **Automatic Verification**: Built-in C reference implementations for correctness checking
- **Golden Sample Caching**: Automatic caching of c_ref outputs for regression testing
- **Kernel Binary Caching**: Compiled OpenCL binaries are cached per algorithm to skip recompilation
- **Performance Benchmarking**: Automatic timing and speedup calculations
- **Flexible Parameters**: OpParams structure supports algorithms with varying requirements
- **Intuitive CLI**: Algorithm selection by name (`./opencl_host dilate3x3 0`)
- **Two-Stage Build**: Build library once, rebuild executable when algorithms change
- **Organized Cache Structure**: Per-algorithm cache organization under `test_data/`

## Project Structure

```
.
├── ARCHITECTURE_v1.md              # Current SDK-ready architecture (Clean Architecture)
├── ARCHITECTURE_v0.md              # Original architecture (reference)
├── README.md                       # This file
├── CMakeLists.txt                  # CMake build configuration
├── config/
│   ├── inputs.json                 # Global input image definitions
│   ├── outputs.json                # Global output image definitions
│   ├── dilate3x3.json              # Dilate algorithm config
│   ├── gaussian5x5.json            # Gaussian algorithm config
│   └── template.json               # Template for new algorithms
├── docs/
│   ├── ADD_NEW_ALGO.md             # Algorithm development guide
│   ├── CONFIG_SYSTEM.md            # Configuration system guide
│   ├── CLEAN_ARCHITECTURE_ANALYSIS.md  # Architecture compliance analysis
│   ├── SDK_PACKAGING.md            # SDK distribution guide
│   └── Doxyfile                    # Doxygen documentation config
├── scripts/
│   ├── build.sh                    # Build script (--lib, --clean options)
│   ├── create_sdk.sh               # SDK packaging script
│   ├── create_new_algo.sh          # Generate new algorithm template
│   ├── generate_registry.sh        # Auto-generate algorithm registry
│   ├── generate_test_image.py      # Test image generator
│   └── run.sh                      # Interactive run script
├── lib/                            # 📦 Release library directory
│   ├── README.md                   # Library release documentation
│   └── libopencl_imgproc.a/.so     # Compiled library (for SDK distribution)
├── include/                        # ✅ Public API (Stable Interface)
│   ├── op_interface.h              # Algorithm interface
│   ├── op_registry.h               # Registration macros
│   ├── algorithm_runner.h          # Forward declarations
│   └── utils/                      # Public utilities
│       ├── safe_ops.h              # Safe arithmetic operations
│       └── verify.h                # Verification functions
├── examples/                       # 👤 User Algorithm Implementations
│   ├── gaussian/
│   │   ├── c_ref/gaussian5x5_ref.c # CPU reference + registration
│   │   └── cl/*.cl                 # GPU kernel variants
│   └── dilate/
│       ├── c_ref/dilate3x3_ref.c   # CPU reference + registration
│       └── cl/*.cl                 # GPU kernel variants
├── src/                            # 🔒 Library Implementation (Internal)
│   ├── main.c                      # Application entry point
│   ├── core/                       # Business Logic
│   │   ├── algorithm_runner.c      # Execution pipeline
│   │   ├── op_registry.c           # Algorithm registry
│   │   └── auto_registry.c         # Auto-generated (don't edit)
│   ├── platform/                   # OpenCL Abstraction
│   │   ├── opencl_utils.c/.h       # Platform initialization
│   │   ├── cache_manager.c/.h      # Binary & golden caching
│   │   └── cl_extension_api.c/.h   # Custom host API
│   └── utils/                      # Infrastructure
│       ├── config.c/.h             # Configuration parser
│       ├── image_io.c/.h           # Image I/O
│       └── verify.c                # Verification implementation
└── test_data/
    ├── dilate3x3/
    └── gaussian5x5/
```

**Key Directories:**
- **`lib/`**: Release artifacts - compiled library ready for distribution
- **`include/`**: Public API - stable interface for users
- **`examples/`**: User algorithms - extend without modifying library
- **`src/`**: Library implementation - core, platform, utils (packaged as library)

**Build artifacts** (`build/`) are created during compilation and can be cleaned with `./scripts/build.sh --clean`.

## Advanced Usage

### Build Options

```bash
./scripts/build.sh                # Incremental build (executable + examples)
./scripts/build.sh --lib          # Build library only (SDK workflow)
./scripts/build.sh --clean        # Clean build (removes build/ and cache/)
./scripts/build.sh --help         # Show help
```

**SDK Workflow** (Two-stage build):
```bash
# Step 1: Build library once (when core/platform/utils change)
./scripts/build.sh --lib
# Output: lib/libopencl_imgproc.so (ready for distribution)

# Step 2: Build executable (when examples change)
./scripts/build.sh
# Output: build/opencl_host (library unchanged, fast rebuild)
```

Or build manually with CMake:
```bash
# Configure (first time only)
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build library only
cmake --build . --target opencl_imgproc

# Build everything (library + executable)
cmake --build .

# Build shared library for SDK
cmake -DBUILD_SHARED_LIBS=ON ..
cmake --build . --target opencl_imgproc
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
./build/opencl_host config/dilate3x3.json 0
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
  - Per-algorithm `.json` file format
  - Configuration parameters
  - Custom buffers and scalars
  - Multiple kernel variants
  - Config-driven kernel arguments

### 🏗️ Architecture & Design

Deep dive into the framework architecture:

- **[ARCHITECTURE_v1.md](ARCHITECTURE_v1.md)** - Current SDK-ready architecture (Clean Architecture)
  - High-level system architecture
  - Module organization and dependencies
  - SDK distribution model
  - Clean Architecture compliance analysis

- **[ARCHITECTURE_v0.md](ARCHITECTURE_v0.md)** - Original architecture (reference)

- **[docs/CLEAN_ARCHITECTURE_ANALYSIS.md](docs/CLEAN_ARCHITECTURE_ANALYSIS.md)** - Architecture compliance analysis
  - Layer hierarchy and dependencies
  - Stability metrics
  - Testability analysis

- **[docs/SDK_PACKAGING.md](docs/SDK_PACKAGING.md)** - SDK distribution guide
  - How to package as library
  - Customer workflow
  - Distribution models
