# OpenCL Image Processing Framework - User Guide

> **Objective**: Enable kernel developers to easily run, explore, and compare OpenCL optimizations across different platforms (standard GPU vs custom CL extension hardware).

---

## Table of Contents

**Part I: Running & Exploring OpenCL Optimizations**
1. [Introduction](#1-introduction)
2. [Quick Start](#2-quick-start)
3. [Running Algorithms & Variants](#3-running-algorithms--variants)
4. [Platform Comparison: Standard CL vs CL Extension](#4-platform-comparison-standard-cl-vs-cl-extension)
5. [Understanding JSON Configuration](#5-understanding-json-configuration)
6. [Reading Performance Results](#6-reading-performance-results)

**Part II: Host Architecture & Binary Generation** *(Conceptual)*
7. [OpenCL Host Flow Overview](#7-opencl-host-flow-overview)
8. [Phase 1: Binary Generation](#8-phase-1-binary-generation)
9. [Phase 2: Execution](#9-phase-2-execution)
10. [Kernel Developer Workflow](#10-kernel-developer-workflow)

**Appendices**
- [A. Adding New Algorithms](#appendix-a-adding-new-algorithms)
- [B. Configuration Reference](#appendix-b-configuration-reference)
- [C. Troubleshooting](#appendix-c-troubleshooting)

---

# Part I: Running & Exploring OpenCL Optimizations

## 1. Introduction

### What This Framework Does

This framework provides a **configuration-driven approach** to running OpenCL image processing algorithms. Instead of writing boilerplate host code, you:

1. **Write your kernel** (`.cl` file)
2. **Configure via JSON** (input/output, work sizes, arguments)
3. **Run and compare** different optimization variants

### Key Benefits for Kernel Developers

| Benefit | Description |
|---------|-------------|
| **Zero host code** | The framework handles all OpenCL host operations |
| **Instant comparison** | Run multiple kernel variants, see speedup instantly |
| **Platform switching** | Compare standard GPU vs CL extension with a config change |
| **Auto-verification** | GPU output automatically verified against CPU reference |
| **Binary caching** | Compiled kernels cached for fast re-runs |

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                         User Space (You focus here)                 │
├─────────────────────────────────────────────────────────────────────┤
│  kernel.cl          config.json           c_ref.c                   │
│  (GPU kernel)       (configuration)       (CPU reference)           │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    Framework (We handle this)                       │
├─────────────────────────────────────────────────────────────────────┤
│  • OpenCL initialization     • Kernel compilation                   │
│  • Memory management         • Argument binding                     │
│  • Execution & timing        • Verification                         │
│  • Binary caching            • Result reporting                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. Quick Start

### Prerequisites

- **OpenCL runtime** (macOS: built-in, Linux: GPU driver, Windows: GPU SDK)
- **CMake 3.10+**
- **C compiler** (gcc, clang)

### Build

```bash
# Clone and build
cd opencl-study
./scripts/build.sh
```

### Run Your First Algorithm

```bash
# Run dilate3x3 algorithm, variant 0 (standard OpenCL)
./build/opencl_host dilate3x3 0
```

### Expected Output

```
=== Algorithm: dilate3x3 ===
Platform: Apple / Apple M1
Running variant: v0 (standard OpenCL)

Reference (CPU):  12.45 ms
GPU Execution:     0.82 ms
Speedup:          15.2x

Verification: PASSED (0 errors)
Output saved to: out/dilate3x3_v0.bin
```

---

## 3. Running Algorithms & Variants

### Available Algorithms

| Algorithm | Description | Variants |
|-----------|-------------|----------|
| `dilate3x3` | 3x3 morphological dilation | v0 (standard), v1 (cl_extension) |
| `gaussian5x5` | 5x5 Gaussian blur | v1f (float), v1, v2, v3 |
| `relu` | ReLU activation function | v0, v1, v3, v6 |

### Understanding Variants

Each algorithm can have **multiple kernel implementations** (variants):

- **v0**: Typically the baseline/standard implementation
- **v1, v2, ...**: Optimized versions with different strategies
- **Suffix meanings**: `v1f` = float version, `v1` = optimized

### CLI Usage

```bash
# Basic syntax
./build/opencl_host <algorithm> <variant>

# Examples
./build/opencl_host dilate3x3 0      # Run dilate, variant v0
./build/opencl_host gaussian5x5 1f   # Run gaussian, variant v1f
./build/opencl_host relu 3           # Run relu, variant v3
```

### Running All Variants

```bash
# Run all variants of all algorithms
./scripts/run_all_variants.sh

# Run all tests with verification
./tests/run_tests.sh
```

### Listing Available Options

```bash
# See what's available
ls config/*.json          # List all algorithm configs
cat config/relu.json      # See variants for relu
```

---

## 4. Platform Comparison: Standard CL vs CL Extension

### What Are the Two Platforms?

| Platform | `host_type` | Description |
|----------|-------------|-------------|
| **Standard** | `"standard"` | Uses standard OpenCL API, works on any GPU |
| **CL Extension** | `"cl_extension"` | Uses vendor-specific custom hardware extensions |

### When to Use Each

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Platform Selection                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   Standard OpenCL                    CL Extension                   │
│   ──────────────                    ────────────                    │
│   • Development & debugging          • Production on target HW      │
│   • Cross-platform testing           • Vendor-specific optimizations│
│   • Algorithm validation             • Custom memory operations     │
│   • Any GPU vendor                   • Specific accelerator         │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Switching Platforms

Platforms are configured **per kernel variant** in JSON:

```json
{
  "kernels": {
    "v0": {
      "description": "standard OpenCL baseline",
      "host_type": "standard",          // ← Standard GPU
      "kernel_file": "examples/relu/cl/relu0.cl"
    },
    "v1": {
      "description": "CL extension optimized",
      "host_type": "cl_extension",      // ← Custom hardware
      "kernel_file": "examples/relu/cl/relu1.cl"
    }
  }
}
```

### Writing Platform-Aware Kernels

Your kernel can include platform-specific utilities:

```opencl
#include "platform.h"   // Platform-aware header

__kernel void my_kernel(...) {
    #if HOST_TYPE == 0  // Standard OpenCL
        // Standard implementation
    #else               // CL Extension
        // Use custom hardware features
        util_mem_copy_2d(...);  // Platform-specific utility
    #endif
}
```

### Comparing Performance

Run the same algorithm with different variants to compare:

```bash
# Compare standard vs cl_extension for dilate3x3
./build/opencl_host dilate3x3 0   # v0: standard
./build/opencl_host dilate3x3 1   # v1: cl_extension

# Example output comparison:
# v0 (standard):      1.2 ms
# v1 (cl_extension):  0.4 ms  ← 3x faster on custom HW
```

---

## 5. Understanding JSON Configuration

### Configuration File Structure

Each algorithm has a JSON config file in `config/`:

```
config/
├── inputs.json        # Global input image definitions
├── outputs.json       # Global output image definitions
├── dilate3x3.json     # Algorithm-specific config
├── gaussian5x5.json
├── relu.json
└── template.json      # Template for new algorithms
```

### Global Input/Output Configuration

**`config/inputs.json`** - Defines input images:
```json
{
  "images": {
    "image_1": {
      "width": 1920,
      "height": 1088,
      "channels": 1,
      "path": "test_data/input.bin"
    }
  }
}
```

**`config/outputs.json`** - Defines output destinations:
```json
{
  "images": {
    "output_1": {
      "path": "out/{op_id}_{variant}.bin"
    }
  }
}
```

### Algorithm Configuration (Example: relu.json)

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
      "description": "standard 2D workgroup",
      "host_type": "standard",
      "kernel_option": "-cl-fast-relaxed-math",
      "kernel_file": "examples/relu/cl/relu0.cl",
      "kernel_function": "relu_kernel",
      "work_dim": 2,
      "global_work_size": [1920, 1088],
      "local_work_size": [16, 16],
      "kernel_args": [
        {"i_buffer": ["uchar", "src"]},
        {"o_buffer": ["uchar", "dst"]},
        {"param": ["int", "width"]},
        {"param": ["int", "height"]}
      ]
    }
  }
}
```

### Key Configuration Sections

#### 1. Input/Output Mapping
```json
{
  "input": { "input_image_id": "image_1" },
  "output": { "output_image_id": "output_1" }
}
```
Links to global `inputs.json` and `outputs.json` definitions.

#### 2. Verification Settings
```json
{
  "verification": {
    "tolerance": 0,              // Per-pixel tolerance (0 = exact match)
    "error_rate_threshold": 0,   // Allowed error percentage (0 = 0%)
    "golden_source": "c_ref"     // Reference: "c_ref" or "file"
  }
}
```

#### 3. Custom Scalars (Algorithm Parameters)
```json
{
  "scalars": {
    "sigma": { "type": "float", "value": 1.4 },
    "kernel_radius": { "type": "int", "value": 2 }
  }
}
```
Passed to kernel as arguments automatically.

#### 4. Custom Buffers
```json
{
  "buffers": {
    "tmp_global": {
      "type": "READ_WRITE",
      "size_bytes": 400000
    },
    "kernel_weights": {
      "type": "READ_ONLY",
      "data_type": "float",
      "num_elements": 25,
      "source_file": "test_data/gaussian5x5/weights.bin"
    }
  }
}
```

#### 5. Kernel Variant Configuration
```json
{
  "kernels": {
    "v0": {
      "description": "Human-readable description",
      "host_type": "standard",           // or "cl_extension"
      "kernel_option": "-cl-fast-relaxed-math",
      "kernel_file": "examples/algo/cl/algo0.cl",
      "kernel_function": "my_kernel",
      "work_dim": 2,
      "global_work_size": [1920, 1088],
      "local_work_size": [16, 16],
      "kernel_args": [...]
    }
  }
}
```

#### 6. Kernel Arguments Format

| Type | Syntax | Description |
|------|--------|-------------|
| Input buffer | `{"i_buffer": ["uchar", "src"]}` | Read-only input image |
| Output buffer | `{"o_buffer": ["uchar", "dst"]}` | Write-only output image |
| Parameter | `{"param": ["int", "width"]}` | Scalar from image metadata |
| Scalar | `{"scalar": ["float", "sigma"]}` | Custom scalar from config |
| Custom buffer | `{"buffer": ["float", "kernel_weights", 100]}` | Custom buffer |

---

## 6. Reading Performance Results

### Output Format

```
=== Algorithm: gaussian5x5 ===
Platform: Apple / Apple M1 Pro
Device: Apple M1 Pro (GPU)
Running variant: v1f (float calculations)

Input:  1920x1088, 1 channel
Output: 1920x1088, 1 channel

Reference (CPU):  45.23 ms
Kernel Compile:    2.10 ms (cached: yes)
GPU Execution:     3.21 ms
Memory Transfer:   1.05 ms

Total GPU Time:    4.26 ms
Speedup:          10.6x

Verification:
  Method: c_ref (CPU reference)
  Tolerance: 1
  Result: PASSED
  Errors: 0 / 2088960 pixels (0.000%)

Output: out/gaussian5x5_v1f.bin
```

### Key Metrics Explained

| Metric | Description |
|--------|-------------|
| **Reference (CPU)** | Time for C reference implementation |
| **GPU Execution** | Time for kernel execution only |
| **Speedup** | CPU time / GPU time |
| **Tolerance** | Allowed per-pixel difference |
| **Errors** | Pixels exceeding tolerance |

### Comparing Variants

```bash
# Run multiple variants and compare
for v in 0 1 2 3; do
  ./build/opencl_host gaussian5x5 $v 2>/dev/null | grep -E "(variant|Speedup)"
done

# Output:
# Running variant: v1f   Speedup: 10.6x
# Running variant: v1    Speedup: 12.3x
# Running variant: v2    Speedup: 14.8x  ← Best
# Running variant: v3    Speedup: 11.2x
```

---

# Part II: Host Architecture & Binary Generation

> **Note**: This section describes the **conceptual architecture** for a two-phase workflow (compile + execute) that is **not yet fully implemented**. Current bottlenecks are marked.

## 7. OpenCL Host Flow Overview

### Current vs Target Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    CURRENT ARCHITECTURE                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   kernel.cl + config.json                                           │
│          │                                                          │
│          ▼                                                          │
│   ┌─────────────────────────────────────────┐                       │
│   │         opencl_host (monolithic)        │                       │
│   │  • Compile kernel (JIT)                 │                       │
│   │  • Execute kernel                       │                       │
│   │  • Verify results                       │                       │
│   └─────────────────────────────────────────┘                       │
│          │                                                          │
│          ▼                                                          │
│       result                                                        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

                              │
                              ▼

┌─────────────────────────────────────────────────────────────────────┐
│                    TARGET ARCHITECTURE                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   PHASE 1: Binary Generation (Offline)                              │
│   ─────────────────────────────────────                             │
│   kernel.cl + config.json                                           │
│          │                                                          │
│          ▼                                                          │
│   ┌─────────────────────────────────────────┐                       │
│   │         cl_compiler (new)               │                       │
│   │  • Parse config                         │                       │
│   │  • Compile kernel to binary             │                       │
│   │  • Generate host configuration          │                       │
│   └─────────────────────────────────────────┘                       │
│          │                                                          │
│          ▼                                                          │
│   program.bin + host_config.bin                                     │
│                                                                     │
│   ─────────────────────────────────────────────────────────────     │
│                                                                     │
│   PHASE 2: Execution (Runtime)                                      │
│   ────────────────────────────                                      │
│   program.bin + host_config.bin + input_data                        │
│          │                                                          │
│          ▼                                                          │
│   ┌─────────────────────────────────────────┐                       │
│   │         cl_executor (new)               │                       │
│   │  • Load pre-compiled binary             │                       │
│   │  • Execute with input data              │                       │
│   │  • Return results                       │                       │
│   └─────────────────────────────────────────┘                       │
│          │                                                          │
│          ▼                                                          │
│       result                                                        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Two-Phase Model Benefits

| Benefit | Description |
|---------|-------------|
| **Faster iteration** | Compile once, run many times |
| **Deployment ready** | Ship binaries without source |
| **Platform-specific** | Pre-compile for target hardware |
| **Smaller runtime** | Executor doesn't need compiler |

---

## 8. Phase 1: Binary Generation

### Conceptual Workflow

```
┌─────────────────────────────────────────────────────────────────────┐
│                   PHASE 1: BINARY GENERATION                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   INPUTS                                                            │
│   ──────                                                            │
│   ┌──────────────┐     ┌──────────────┐                             │
│   │  kernel.cl   │     │ config.json  │                             │
│   │              │     │              │                             │
│   │ __kernel     │     │ work_size    │                             │
│   │ void foo(){} │     │ kernel_args  │                             │
│   └──────┬───────┘     └──────┬───────┘                             │
│          │                    │                                     │
│          └────────┬───────────┘                                     │
│                   ▼                                                 │
│   PROCESSING                                                        │
│   ──────────                                                        │
│   ┌─────────────────────────────────────────┐                       │
│   │           cl_compiler                   │                       │
│   ├─────────────────────────────────────────┤                       │
│   │ 1. Parse config.json                    │                       │
│   │    - Extract kernel function name       │                       │
│   │    - Extract work dimensions            │                       │
│   │    - Extract argument specifications    │                       │
│   │                                         │                       │
│   │ 2. Compile kernel.cl                    │                       │
│   │    - OpenCL offline compilation         │                       │
│   │    - Target-specific optimizations      │                       │
│   │    - Generate platform binary           │                       │
│   │                                         │                       │
│   │ 3. Generate host configuration          │                       │
│   │    - Serialize argument bindings        │                       │
│   │    - Serialize work size info           │                       │
│   │    - Create binary metadata             │                       │
│   └─────────────────────────────────────────┘                       │
│                   │                                                 │
│                   ▼                                                 │
│   OUTPUTS                                                           │
│   ───────                                                           │
│   ┌──────────────┐     ┌──────────────┐                             │
│   │ program.bin  │     │host_config   │                             │
│   │              │     │   .bin       │                             │
│   │ Compiled     │     │              │                             │
│   │ OpenCL       │     │ Runtime      │                             │
│   │ binary       │     │ config       │                             │
│   └──────────────┘     └──────────────┘                             │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### What Gets Generated

| Output | Contents | Purpose |
|--------|----------|---------|
| `program.bin` | Compiled OpenCL kernel | GPU execution |
| `host_config.bin` | Serialized configuration | Argument binding at runtime |

### Proposed CLI

```bash
# Compile kernel for target platform
cl_compiler --kernel relu0.cl \
            --config relu.json \
            --variant v0 \
            --platform standard \
            --output build/relu_v0

# Outputs:
# build/relu_v0.program.bin    (compiled kernel)
# build/relu_v0.host.bin       (host configuration)
```

### Current Implementation Status

| Component | Status | Notes |
|-----------|--------|-------|
| Kernel compilation | ✅ Exists | `src/platform/opencl_utils.c` |
| Binary caching | ✅ Exists | `src/platform/cache_manager.c` |
| Config parsing | ✅ Exists | `src/utils/config.c` |
| Standalone compiler | ❌ **Not Implemented** | Combined with executor |
| Host config serialization | ❌ **Not Implemented** | Config parsed at runtime |

### Bottlenecks & Required Work

```
┌─────────────────────────────────────────────────────────────────────┐
│                    BOTTLENECK ANALYSIS                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│ BOTTLENECK 1: No Standalone Compiler                                │
│ ────────────────────────────────────                                │
│ Current: Compilation happens inside opencl_host at runtime          │
│ Needed:  Separate cl_compiler executable                            │
│ Effort:  Extract compilation logic from algorithm_runner.c          │
│          Create new main_compiler.c entry point                     │
│                                                                     │
│ BOTTLENECK 2: Config Not Serialized                                 │
│ ──────────────────────────────────                                  │
│ Current: JSON parsed at runtime every execution                     │
│ Needed:  Binary format for host configuration                       │
│ Effort:  Design host_config.bin format                              │
│          Add serialization to config.c                              │
│          Add deserialization for executor                           │
│                                                                     │
│ BOTTLENECK 3: Argument Binding Tied to Runtime                      │
│ ─────────────────────────────────────────────                       │
│ Current: kernel_args.c binds arguments during execution             │
│ Needed:  Pre-computed argument layout in host_config.bin            │
│ Effort:  Separate binding preparation from execution                │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 9. Phase 2: Execution

### Conceptual Workflow

```
┌─────────────────────────────────────────────────────────────────────┐
│                     PHASE 2: EXECUTION                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   INPUTS                                                            │
│   ──────                                                            │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│   │ program.bin  │  │host_config   │  │ input_data   │              │
│   │              │  │   .bin       │  │              │              │
│   │ Pre-compiled │  │ Runtime      │  │ Image or     │              │
│   │ kernel       │  │ config       │  │ buffer       │              │
│   └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │
│          │                 │                 │                      │
│          └─────────────────┼─────────────────┘                      │
│                            ▼                                        │
│   PROCESSING                                                        │
│   ──────────                                                        │
│   ┌─────────────────────────────────────────┐                       │
│   │           cl_executor                   │                       │
│   ├─────────────────────────────────────────┤                       │
│   │ 1. Initialize OpenCL                    │                       │
│   │    - Create context & queue             │                       │
│   │    - NO compilation needed              │                       │
│   │                                         │                       │
│   │ 2. Load pre-compiled binary             │                       │
│   │    - clCreateProgramWithBinary()        │                       │
│   │    - Instant kernel ready               │                       │
│   │                                         │                       │
│   │ 3. Bind arguments (from host_config)    │                       │
│   │    - Create buffers                     │                       │
│   │    - Upload input data                  │                       │
│   │    - Set kernel arguments               │                       │
│   │                                         │                       │
│   │ 4. Execute                              │                       │
│   │    - clEnqueueNDRangeKernel()           │                       │
│   │    - Wait for completion                │                       │
│   │                                         │                       │
│   │ 5. Read results                         │                       │
│   │    - Download output buffer             │                       │
│   │    - Return to caller                   │                       │
│   └─────────────────────────────────────────┘                       │
│                            │                                        │
│                            ▼                                        │
│   OUTPUT                                                            │
│   ──────                                                            │
│   ┌──────────────┐                                                  │
│   │   result     │                                                  │
│   │              │                                                  │
│   │ Processed    │                                                  │
│   │ image/data   │                                                  │
│   └──────────────┘                                                  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Proposed CLI

```bash
# Execute pre-compiled kernel
cl_executor --program build/relu_v0.program.bin \
            --config build/relu_v0.host.bin \
            --input test_data/input.bin \
            --output out/result.bin

# Or integrated mode (current behavior)
opencl_host relu 0 --input custom_input.bin
```

### Binary Sharing Between Phases

The key benefit is **compile once, run many times**:

```
                    DEVELOPMENT                    PRODUCTION
                    ───────────                    ──────────

kernel.cl ──┐
            │
config.json ┼──► cl_compiler ──► program.bin ──┬──► cl_executor ──► result1
            │                    host.bin    │
(one time)  │                                ├──► cl_executor ──► result2
                                             │
                                             └──► cl_executor ──► result3
                                                  (many times)
```

### Current Implementation Status

| Component | Status | Notes |
|-----------|--------|-------|
| OpenCL initialization | ✅ Exists | `src/platform/opencl_utils.c` |
| Binary loading | ✅ Exists | `clCreateProgramWithBinary` in cache_manager |
| Argument binding | ✅ Exists | `src/platform/kernel_args.c` |
| Execution | ✅ Exists | Part of algorithm_runner.c |
| Standalone executor | ❌ **Not Implemented** | Combined with compiler |
| Binary config loading | ❌ **Not Implemented** | Uses JSON at runtime |

### Bottlenecks & Required Work

```
┌─────────────────────────────────────────────────────────────────────┐
│                    BOTTLENECK ANALYSIS                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│ BOTTLENECK 4: No Standalone Executor                                │
│ ────────────────────────────────────                                │
│ Current: Execution coupled with compilation in opencl_host          │
│ Needed:  Separate cl_executor that only runs pre-compiled binaries  │
│ Effort:  Create main_executor.c entry point                         │
│          Skip compilation path when binary exists                   │
│                                                                     │
│ BOTTLENECK 5: Runtime Config Dependency                             │
│ ───────────────────────────────────────                             │
│ Current: Executor needs original JSON config files                  │
│ Needed:  Executor reads from host_config.bin only                   │
│ Effort:  Design self-contained binary format                        │
│          Include all runtime info in host_config.bin                │
│                                                                     │
│ BOTTLENECK 6: Input/Output Path Hardcoding                          │
│ ──────────────────────────────────────────                          │
│ Current: Paths defined in global inputs.json/outputs.json           │
│ Needed:  CLI-specified input/output for flexible execution          │
│ Effort:  Add --input/--output CLI flags to executor                 │
│          Make paths overridable at runtime                          │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 10. Kernel Developer Workflow

### Current Workflow (What Works Today)

```
┌─────────────────────────────────────────────────────────────────────┐
│                CURRENT KERNEL DEVELOPER WORKFLOW                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│ STEP 1: Create Algorithm Structure                                  │
│ ──────────────────────────────────                                  │
│                                                                     │
│   $ ./scripts/create_new_algo.sh myalgo "My Algorithm"              │
│                                                                     │
│   Generated:                                                        │
│   ├── config/myalgo.json                                            │
│   ├── examples/myalgo/c_ref/myalgo_ref.c                            │
│   └── examples/myalgo/cl/myalgo0.cl                                 │
│                                                                     │
│                                                                     │
│ STEP 2: Implement C Reference                                       │
│ ─────────────────────────────                                       │
│                                                                     │
│   // examples/myalgo/c_ref/myalgo_ref.c                             │
│   void MyalgoRef(const OpParams* params) {                          │
│       // CPU implementation for verification                        │
│       for (int y = 0; y < height; y++) {                            │
│           for (int x = 0; x < width; x++) {                         │
│               output[y*width + x] = process(input[y*width + x]);    │
│           }                                                         │
│       }                                                             │
│   }                                                                 │
│                                                                     │
│                                                                     │
│ STEP 3: Implement OpenCL Kernel                                     │
│ ───────────────────────────────                                     │
│                                                                     │
│   // examples/myalgo/cl/myalgo0.cl                                  │
│   __kernel void myalgo_kernel(                                      │
│       __global const uchar* input,                                  │
│       __global uchar* output,                                       │
│       int width, int height)                                        │
│   {                                                                 │
│       int x = get_global_id(0);                                     │
│       int y = get_global_id(1);                                     │
│       output[y*width + x] = process(input[y*width + x]);            │
│   }                                                                 │
│                                                                     │
│                                                                     │
│ STEP 4: Configure JSON                                              │
│ ──────────────────────                                              │
│                                                                     │
│   // config/myalgo.json                                             │
│   {                                                                 │
│     "kernels": {                                                    │
│       "v0": {                                                       │
│         "host_type": "standard",                                    │
│         "kernel_function": "myalgo_kernel",                         │
│         "global_work_size": [1920, 1088],                           │
│         "local_work_size": [16, 16],                                │
│         ...                                                         │
│       }                                                             │
│     }                                                               │
│   }                                                                 │
│                                                                     │
│                                                                     │
│ STEP 5: Build and Run                                               │
│ ─────────────────────                                               │
│                                                                     │
│   $ ./scripts/build.sh                                              │
│   $ ./build/opencl_host myalgo 0                                    │
│                                                                     │
│   Output:                                                           │
│   Reference (CPU):  XX.XX ms                                        │
│   GPU Execution:    X.XX ms                                         │
│   Speedup:          XX.Xx                                           │
│   Verification:     PASSED                                          │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### What You Focus On vs What Framework Handles

```
┌─────────────────────────────────────────────────────────────────────┐
│                   RESPONSIBILITY SEPARATION                         │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   KERNEL DEVELOPER (You)              FRAMEWORK (We Handle)         │
│   ──────────────────────              ─────────────────────         │
│                                                                     │
│   ✍️  Write kernel.cl                  🔧 OpenCL initialization      │
│       - Algorithm logic               🔧 Context & queue creation   │
│       - Optimization strategies       🔧 Memory allocation          │
│                                       🔧 Buffer creation            │
│   ✍️  Write c_ref.c                                                 │
│       - Reference implementation      🔧 Kernel compilation         │
│       - Verification baseline         🔧 Binary caching             │
│                                                                     │
│   ✍️  Configure JSON                   🔧 Argument binding           │
│       - Work sizes                    🔧 Data upload/download       │
│       - Argument specifications       🔧 Execution dispatch         │
│                                                                     │
│   ✍️  Add optimization variants        🔧 Verification               │
│       - v0, v1, v2 kernels            🔧 Performance timing         │
│       - Different strategies          🔧 Result comparison          │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Target Workflow (Future with Two-Phase)

```
┌─────────────────────────────────────────────────────────────────────┐
│                  TARGET KERNEL DEVELOPER WORKFLOW                   │
│                        (Not Yet Implemented)                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│ DEVELOPMENT PHASE (on dev machine)                                  │
│ ──────────────────────────────────                                  │
│                                                                     │
│   1. Write kernel and config (same as today)                        │
│                                                                     │
│   2. Compile for target platform:                                   │
│      $ cl_compiler --kernel myalgo.cl \                             │
│                    --config myalgo.json \                           │
│                    --platform cl_extension \                        │
│                    --output dist/myalgo                             │
│                                                                     │
│   3. Test locally:                                                  │
│      $ cl_executor --program dist/myalgo.bin \                      │
│                    --input test.bin                                 │
│                                                                     │
│                                                                     │
│ DEPLOYMENT PHASE (on target hardware)                               │
│ ─────────────────────────────────────                               │
│                                                                     │
│   Ship only:                                                        │
│   ├── dist/myalgo.program.bin   (compiled kernel)                   │
│   ├── dist/myalgo.host.bin      (host config)                       │
│   └── cl_executor               (runtime binary)                    │
│                                                                     │
│   No source code needed on target!                                  │
│                                                                     │
│   Run:                                                              │
│   $ cl_executor --program myalgo.program.bin \                      │
│                 --input camera_frame.bin \                          │
│                 --output processed.bin                              │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

# Appendices

## Appendix A: Adding New Algorithms

### Quick Method (Recommended)

```bash
# Generate boilerplate
./scripts/create_new_algo.sh myalgo "My Algorithm Description"

# Edit generated files
vim examples/myalgo/c_ref/myalgo_ref.c
vim examples/myalgo/cl/myalgo0.cl
vim config/myalgo.json

# Build and test
./scripts/build.sh
./build/opencl_host myalgo 0
```

### Manual Method

1. Create directory structure:
   ```
   examples/myalgo/
   ├── c_ref/myalgo_ref.c
   └── cl/myalgo0.cl
   ```

2. Create config file: `config/myalgo.json`

3. Implement C reference with correct signature:
   ```c
   void MyalgoRef(const OpParams* params) {
       // Access params->input_buffer, params->output_buffer, etc.
   }
   ```

4. Implement OpenCL kernel:
   ```opencl
   __kernel void myalgo_kernel(__global const uchar* in,
                               __global uchar* out,
                               int w, int h) {
       // Implementation
   }
   ```

5. Build: `./scripts/build.sh`

See full guide: [docs/ADD_NEW_ALGO.md](ADD_NEW_ALGO.md)

---

## Appendix B: Configuration Reference

### Complete JSON Schema

```json
{
  "input": {
    "input_image_id": "image_1"        // Reference to inputs.json
  },
  "output": {
    "output_image_id": "output_1"      // Reference to outputs.json
  },
  "verification": {
    "tolerance": 0,                    // Per-pixel tolerance (int)
    "error_rate_threshold": 0.0,       // Allowed error % (float)
    "golden_source": "c_ref",          // "c_ref" or "file"
    "golden_file": "path/to/golden.bin" // If golden_source="file"
  },
  "scalars": {                         // Optional custom parameters
    "param_name": {
      "type": "float|int",
      "value": 1.5
    }
  },
  "buffers": {                         // Optional custom buffers
    "buffer_name": {
      "type": "READ_ONLY|WRITE_ONLY|READ_WRITE",
      "size_bytes": 1024,              // OR
      "data_type": "float",
      "num_elements": 256,
      "source_file": "path/to/data.bin" // Optional: pre-load data
    }
  },
  "kernels": {
    "v0": {
      "description": "Variant description",
      "host_type": "standard|cl_extension",
      "kernel_option": "-cl-fast-relaxed-math",
      "kernel_file": "examples/algo/cl/algo0.cl",
      "kernel_function": "kernel_entry_point",
      "work_dim": 1|2|3,
      "global_work_size": [1920, 1088],
      "local_work_size": [16, 16],
      "kernel_args": [
        {"i_buffer": ["uchar", "src"]},      // Input buffer
        {"o_buffer": ["uchar", "dst"]},      // Output buffer
        {"param": ["int", "src_width"]},     // Auto param
        {"scalar": ["float", "sigma"]},      // Custom scalar
        {"buffer": ["float", "weights", 25]} // Custom buffer
      ]
    }
  }
}
```

### Supported Data Types

| Type | C Type | Size |
|------|--------|------|
| `uchar` | `unsigned char` | 1 byte |
| `short` | `short` | 2 bytes |
| `int` | `int` | 4 bytes |
| `float` | `float` | 4 bytes |
| `size_t` | `size_t` | 4/8 bytes |

### Auto-Generated Parameters

These parameters are automatically available based on input image:

| Name | Description |
|------|-------------|
| `src_width` | Input image width |
| `src_height` | Input image height |
| `dst_width` | Output image width |
| `dst_height` | Output image height |
| `width` | Alias for src_width |
| `height` | Alias for src_height |

---

## Appendix C: Troubleshooting

### Common Issues

#### Build Fails: "OpenCL not found"
```bash
# macOS: Built-in, should work
# Linux: Install OpenCL ICD
sudo apt install ocl-icd-opencl-dev

# Check OpenCL devices
clinfo
```

#### "No OpenCL devices found"
- Ensure GPU drivers are installed
- Check if OpenCL runtime is available:
  ```bash
  ls /etc/OpenCL/vendors/
  ```

#### Verification Failed
- Check tolerance setting in config
- Floating-point algorithms may need `"tolerance": 1`
- Run with verbose mode to see per-pixel differences

#### Kernel Compilation Error
- Check kernel syntax with online OpenCL validator
- Review compiler options in config
- Check for platform-specific features (cl_extension)

#### Performance Worse Than Expected
- Verify work group size fits hardware limits
- Check memory access patterns (coalesced access)
- Try different variants to find optimal

### Debug Commands

```bash
# Verbose build
./scripts/build.sh 2>&1 | tee build.log

# Check available algorithms
ls config/*.json

# Check generated registry
cat src/core/auto_registry.c

# Run tests with detailed output
./tests/run_tests.sh --verbose
```

---

## Summary: Key Takeaways

### For Kernel Developers (Part I)

1. **Write kernel** → `examples/{algo}/cl/{algo}0.cl`
2. **Write reference** → `examples/{algo}/c_ref/{algo}_ref.c`
3. **Configure** → `config/{algo}.json`
4. **Run** → `./build/opencl_host {algo} {variant}`

### For Understanding Host Flow (Part II)

| Current | Future |
|---------|--------|
| Monolithic `opencl_host` | `cl_compiler` + `cl_executor` |
| JIT compilation | Offline compilation |
| JSON config at runtime | Binary config |
| Source code needed | Binaries only |

**Key bottlenecks for two-phase implementation:**
1. Separate compiler executable needed
2. Binary configuration format needed
3. Standalone executor needed

---

*Generated for opencl-study framework*
