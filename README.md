# OpenCL Study Project

A clean, metadata-driven framework for studying OpenCL development with minimal boilerplate.

## Overview

This framework demonstrates how to build scalable OpenCL applications using a **separation of concerns** approach:
- **Operations** focus on business logic - providing simple metadata about their inputs, outputs, and kernels
- **main.cpp** handles all OpenCL infrastructure - platform setup, buffer management, data transfer, kernel execution
- **OpBase** defines the interface for operations to provide their specifications

Result: New operations require only ~50 lines of descriptors instead of 100+ lines of OpenCL boilerplate.

## Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Host code with operation selection
│   ├── ops/                # Operation implementations
│   │   ├── opBase.h        # Base class (header-only)
│   │   ├── threshold.h     # Threshold operation declaration
│   │   └── threshold.cpp   # Threshold operation implementation
│   └── cl/                 # OpenCL kernel sources
│       ├── threshold.cl    # Threshold kernel
│       └── foo.cl          # Default kernel template
├── scripts/
│   ├── build.sh            # Build script
│   └── run.sh              # Run script with operation selection
└── docs/
    ├── opencl_concepts.md  # Detailed OpenCL concepts explanation
    └── STUDY_PLAN.md       # Step-by-step learning guide
```

## Framework Architecture

The project uses a clean, metadata-driven design that separates concerns and keeps operations simple:

### OpBase (Header-only Interface)

- **Defines the interface** for operations via pure virtual methods:
  - `getName()` - Operation name
  - `getKernelPath()` - Path to kernel file (default: kernels/foo.cl)
  - `getKernelName()` - Kernel function name
  - `prepareInputData()` - Load/create input data
  - `getInputBufferSpec()` - Return input buffer specification (pointer, size, flags)
  - `getOutputBufferSpec()` - Return output buffer specification (pointer, size, flags)
  - `getKernelArguments()` - Return kernel argument specifications (buffers and scalars)
  - `getGlobalWorkSize()` - Return work dimensions
  - `verifyResults()` - Validate against golden images (optional)
- **Provides helper utilities**:
  - `loadKernelSource()` - Static helper to load kernel files

### main.cpp (Host Program)

- **Handles complete OpenCL workflow** (Steps 1-8)
  - Platform/device/context setup (Steps 1-2)
  - Kernel loading and building (Step 3)
  - Buffer allocation and data transfer (Steps 5-6)
  - Kernel argument configuration and execution (Step 7)
  - Result retrieval (Step 8)
  - Error handling and resource cleanup
- **Manages operation list** via registry - Operations auto-register
- **Supports operation selection** - Command-line or interactive
- **Uses operation specifications** to configure all OpenCL operations

### Derived Operations

Each operation (e.g., ThresholdOp) provides simple metadata descriptors. Operations focus ONLY on:
- **Inputs/outputs** - Preparing data and providing buffer specifications
- **Kernel metadata** - Kernel path, name, and argument specifications
- **Verification** - Testing results (optional)

All OpenCL flow control is handled by main.cpp using the specifications provided by operations.

### Responsibility Division

| Responsibility | Who Handles It | How |
|---------------|----------------|-----|
| **Platform/device setup** | main.cpp | Steps 1-2 |
| **Kernel loading** | main.cpp | Step 3 (uses `getKernelPath()`) |
| **Data preparation** | Operation | `prepareInputData()` |
| **Buffer allocation** | main.cpp | Step 5 (uses `getInputBufferSpec()`, `getOutputBufferSpec()`) |
| **Data transfer** | main.cpp | Step 6 (uses buffer specs) |
| **Kernel arguments** | main.cpp | Step 7 (uses `getKernelArguments()`) |
| **Kernel execution** | main.cpp | Step 7 (uses `getGlobalWorkSize()`) |
| **Result retrieval** | main.cpp | Step 8 (uses buffer specs) |
| **Verification** | Operation | `verifyResults()` (optional) |
| **Cleanup** | main.cpp | Step 9 (automatic) |

Operations provide ~5-7 simple methods returning metadata. main.cpp does all the OpenCL flow control.

## Building and Running

### Quick Start (Linux/macOS)

```bash
# Build the project
./scripts/build.sh

# Run with operation selection (interactive)
./scripts/run.sh

# Run with specific operation (e.g., 1 = Threshold)
./scripts/run.sh 1
```

## Generating Documentation

The project uses Doxygen for generating comprehensive API documentation from source code comments.

Install Doxygen on your system:

```bash
# macOS
brew install doxygen

# Linux (Ubuntu/Debian)
sudo apt install doxygen graphviz

# Windows
# Download from https://www.doxygen.nl/download.html
```

```bash
# Generate HTML documentation (run from project root)
doxygen docs/Doxyfile
```

This creates documentation in the `docs/html/` directory.

## Further Learning

For more detailed explanations of OpenCL concepts used in this framework, see:
- [docs/opencl_concepts.md](docs/opencl_concepts.md) - In-depth concept explanations
- [OpenCV OpenCL Optimizations](https://github.com/opencv/opencv/wiki/OpenCL-optimizations) - Reference documentation

## Note for macOS Users

Apple deprecated OpenCL in favor of Metal. While this code works on macOS systems with OpenCL support, future macOS versions may not include OpenCL. This project is intended as a study resource for understanding OpenCL concepts that are widely applicable on Linux/Windows systems.
