# OpenCL Image Processing Framework - Slide Plan Part I

> **Presentation Duration**: 30 minutes
> **Objective**: Enable kernel developers to easily run, explore, and compare OpenCL optimizations across different platforms

---

## Slide Overview (Total: 19 slides)

| Section | Slides | Time |
|---------|--------|------|
| 1. Introduction / Features | 1-5 | 5 min |
| 2. General OpenCL Host Flow | 6 | 3 min |
| 3. Standard CL v.s. CL Extension | 7-10 | 7 min |
| 4. Caching Mechanism | 11-12 | 4 min |
| 5. JSON Configuration | 13-17 | 8 min |
| 6. Summary & Q&A | 18-19 | 3 min |

---

## Section 1: Introduction / Features

### Slide 1: Title Slide
**Title**: OpenCL Image Processing Framework
**Subtitle**: Easy Kernel Development, Zero Host Code

**Content**:
- Framework logo/visual
- Presenter name and date
- Tagline: "Focus on your kernels, we handle the rest"

**Speaker Notes**:
Welcome to the OpenCL Image Processing Framework presentation. This framework is designed to make kernel development as simple as possible. You write kernels, we handle all the OpenCL boilerplate.

---

### Slide 2: The Problem We Solve
**Title**: Traditional OpenCL Development Pain Points

**Content** (Bullet points with icons):
- Writing 500+ lines of host code for each algorithm
- Managing OpenCL contexts, queues, buffers manually
- Compiling kernels and handling errors
- Setting up arguments and work sizes
- Verifying results against CPU reference
- Repeating this for every kernel variant

**Visual**: Display `svg/02_problem_solution.svg`

**Speaker Notes**:
Traditional OpenCL development requires significant host-side boilerplate. For each algorithm, you typically write hundreds of lines of setup code. This framework eliminates that overhead.

---

### Slide 3: What This Framework Provides
**Title**: Framework Benefits

**Visual**: Display `svg/03_framework_benefits.svg`

**Content** (Icons + short descriptions):

| Benefit | Description |
|---------|-------------|
| Zero Host Code | JSON-driven configuration |
| Instant Comparison | Run multiple variants |
| Platform Switching | Standard GPU vs CL Extension |
| Auto-Verification | GPU output vs CPU reference |
| Binary Caching | Fast re-runs with cached kernels |
| Easy Extension | Add new algorithms with scaffold script |

**How to Add New Algorithms**:
- Run `./scripts/create_new_algo.sh <name>` to scaffold
- Auto-generates: `examples/<name>/c_ref/<name>_ref.c` + `cl/<name>0.cl` + `config/<name>.json`
- Auto-registry: Build script generates registration code
- Add variants: Just add `v1`, `v2`... to JSON config

**Speaker Notes**:
The framework provides five key benefits. Most importantly, you never write OpenCL host code - everything is configured through JSON files.

---

### Slide 4: User Journey
**Title**: How to Use the Framework

**Visual**: Display `svg/04_user_journey.svg`

**Content** (3-step flow):

| Step | Action | Output |
|------|--------|--------|
| 1. Build | `make` | `opencl_host` executable |
| 2. Run | `./opencl_host <algo> <variant>` | Execution + verification results |
| 3. Learn | Analyze profiler output | Performance insights |

**Objective**: Learn three algorithms with different optimization variants
- dilate3x3, gaussian5x5, relu
- Each has v0, v1, v2... variants with increasing optimization
- Kernel optimization details in separate presentations

**Speaker Notes**:
The workflow is simple: build once, then run any algorithm variant. Use profiler output to understand performance differences between variants.


---

### Slide 5: Architecture Overview
**Title**: High-Level Architecture

**Visual**: Display `svg/05_architecture_overview.svg`

**Key Layers**:
| Layer | Components | Who Handles |
|-------|-----------|-------------|
| User Space | kernel.cl, config.json, c_ref.c | You |
| Framework | OpenCL init, compilation, execution, verification | Framework |

**Speaker Notes**:
Here's the key insight: you work in the upper layer with three files - your kernel, a JSON config, and a C reference. The framework handles everything else below.

---
## Section 2: General OpenCL Host Flow

### Slide 6: OpenCL Host Flow Overview
**Title**: Standard OpenCL Host Flow

**Visual**: Display `svg/06_opencl_host_flow.svg`

**Content** (6-step flow):
1. Platform/Device Discovery
2. Context/Queue Creation
3. Kernel Compilation
4. Buffer Allocation
5. Kernel Execution
6. Result Retrieval

**Speaker Notes**:
This is the standard OpenCL host flow that every OpenCL application must implement. The framework handles all these steps automatically.

---
## Section 3: Standard CL v.s. CL Extension

### Slide 7: CL Extension Branching
**Title**: Where We Branch to CL Extension

**Visual**: Display `svg/07_cl_extension_split.svg`

**Speaker Notes**:
Introduce where we branch out the CL extension part from the standard OpenCL flow.

---

### Slide 8: Two Platforms
**Title**: Standard CL vs CL Extension

**Visual**: Display `svg/08_platform_comparison.svg`

**Content** (Two-column comparison):

| Standard OpenCL | CL Extension |
|-----------------|--------------|
| `host_type: "standard"` | `host_type: "cl_extension"` |
| Works on any GPU | Vendor-specific hardware |
| Standard OpenCL APIs | Custom extension APIs |
| Development & debugging | Production optimization |
| Cross-platform testing | Maximum performance |



**Speaker Notes**:
The framework supports two platform types. Standard OpenCL works everywhere and is great for development. CL Extension uses vendor-specific optimizations for production.

---

### Slide 9: Platform Branching Flow
**Title**: How Platform Selection Works

**Visual**: Display `svg/09_platform_branching.svg`

**Key Points**:
1. Platform configured per-kernel in JSON
2. `host_type` field determines the path
3. Compile-time flag: `-DHOST_TYPE=0|1`
4. Runtime API selection in `OpenclRunKernel()`

**Code Reference**: `opencl_utils.c:586-608`

**Speaker Notes**:
This diagram shows how the framework branches between platforms. The decision happens based on the JSON config, and both compile-time and runtime branching is handled automatically.

---

### Slide 10: Configuring Platforms in JSON
**Title**: Platform Configuration

**Content**:

**In your config.json**:
```json
{
  "kernels": {
    "v0": {
      "description": "Standard OpenCL baseline",
      "host_type": "standard",
      "kernel_file": "examples/relu/cl/relu0.cl"
    },
    "v1": {
      "description": "CL extension optimized",
      "host_type": "cl_extension",
      "kernel_file": "examples/relu/cl/relu1.cl"
    }
  }
}
```

**Note**: Same algorithm, two implementations, different platforms

**Speaker Notes**:
Switching platforms is a one-line change in JSON. You can have both platform variants in the same config file and compare them directly.

---

## Section 4: Caching Mechanism

### Slide 11: Binary Caching
**Title**: Kernel Binary Cache

**Visual**: Display `svg/11_cache_mechanism.svg`

**Key Points**:
- First run: Compile kernel (~100ms)
- Subsequent runs: Load binary (~5ms)
- Hash-based invalidation: Detects source changes
- Cache location: `cache/{algo}_{variant}_{timestamp}/`

**Code Reference**: `cache_manager.c`

**Speaker Notes**:
Introduce how we cache the binaries (standard CL v.s. CL extension). The cache mechanism dramatically speeds up repeated runs. After the first compilation, binaries are cached. If you change the kernel source, the hash check detects this and triggers recompilation.

---

### Slide 12: Custom Binary Cache (CL Extension)
**Title**: Custom Binary Cache Mechanism

**Visual**: Display `svg/12_custom_binary_cache.svg`

**Key Points**:

| Aspect | Standard Cache | Custom Binary Cache |
|--------|---------------|---------------------|
| Binary Type | JIT-compiled (clBuildProgram) | Pre-compiled (vendor-specific) |
| Cache Files | `{kernel}.bin` + `{kernel}.hash` | `{kernel}_custom.bin` |
| Portability | Any OpenCL GPU | Specific hardware target |
| Performance | Good (generic) | Maximum (HW-specific) |

**Flow**:
1. **Compilation Phase (Linux)**: ClExtensionBuildKernel() → CacheSaveCustomBinary()
2. **Execution Phase (Linux/Android)**: Load custom binary → ClExtensionEnqueueNdrange()

**Code Reference**: `cache_manager.c:324 CacheSaveCustomBinary()`, `opencl_utils.c:597`

**Speaker Notes**:
The CL Extension path uses a custom binary cache. Unlike standard JIT compilation, custom binaries are pre-compiled with hardware-specific optimizations. This enables cross-platform deployment: compile once on Linux, execute on Linux or Android with maximum performance.

---
## Section 5: JSON Configuration

### Slide 13: Configuration File Structure
**Title**: JSON Configuration Overview

**Content**:

**File Organization**:
```
config/
├── inputs.json      # Global input definitions
├── outputs.json     # Global output definitions
├── dilate3x3.json   # Algorithm-specific config
├── gaussian5x5.json
├── relu.json
└── template.json    # Template for new algorithms
```

**Relationship**:
- `inputs.json` → Defines test images
- `{algo}.json` → References inputs/outputs, defines kernels

**Speaker Notes**:
Configuration is split across files. Global inputs and outputs are defined once, then referenced by algorithm configs. This promotes reuse and consistency.

---

### Slide 14: Input/Output Configuration
**Title**: Defining Inputs and Outputs

**Content**:

**config/inputs.json**:
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

**config/outputs.json**:
```json
{
  "images": {
    "output_1": {
      "path": "out/{op_id}_{variant}.bin"
    }
  }
}
```

**Speaker Notes**:
Input images define dimensions and file paths. Outputs can use template variables like {op_id} for automatic naming. You can define multiple images for testing.

---

### Slide 15: Kernel Configuration Deep Dive
**Title**: Kernel Definition

**Content**:

**Full Kernel Config Example**:
```json
"v0": {
  "description": "standard with struct params",
  "host_type": "standard",
  "kernel_option": "-cl-fast-relaxed-math",
  "kernel_file": "examples/relu/cl/relu0.cl",
  "kernel_function": "relu",
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
```

**Speaker Notes**:
This is a complete kernel configuration. Notice how work sizes, compiler options, and kernel arguments are all specified in JSON - no host code needed.

---

### Slide 16: Kernel Argument Types
**Title**: Supported Argument Types

**Content** (Table):

| Type | Syntax | Description |
|------|--------|-------------|
| Input Buffer | `{"i_buffer": ["uchar", "src"]}` | Read-only image |
| Output Buffer | `{"o_buffer": ["uchar", "dst"]}` | Write-only result |
| Parameter | `{"param": ["int", "width"]}` | Auto from image |
| Scalar | `{"scalar": ["float", "sigma"]}` | Custom value |
| Custom Buffer | `{"buffer": ["float", "weights", 100]}` | User-defined |
| Struct | `{"struct": ["a", "b", "c"]}` | Packed scalars |

**Scalars defined in config**:
```json
"scalars": {
  "sigma": {"type": "float", "value": 1.4}
}
```

**Speaker Notes**:
The framework supports six argument types. Parameters are automatically extracted from image metadata. Scalars and custom buffers let you pass additional data to your kernels.

---

### Slide 17: Verification Settings
**Title**: Automatic Verification

**Content**:

**Verification Config**:
```json
"verification": {
  "tolerance": 0,
  "golden_source": "c_ref"
}
```

**Settings Explained**:
- `tolerance`: Per-pixel difference allowed (0 = exact match)
- `golden_source`: `"c_ref"` (CPU reference) or `"file"` (golden file)

**Use Cases**:
- Exact match: `tolerance: 0`
- Approximate: `error_rate_threshold: 0.01` (1% errors OK)

**Speaker Notes**:
Verification is automatic. Configure tolerance for floating-point algorithms. The framework compares GPU output against C reference or golden file automatically.

---

## Section 6: Summary & Q&A

### Slide 18: Summary
**Title**: Key Takeaways

**Content**:

| What You Learned | Key Points |
|------------------|------------|
| Framework Benefits | Zero host code, JSON-driven configuration |
| User Journey | Build → Run → Learn from profiler |
| OpenCL Host Flow | 6-step flow handled by framework |
| Platform Options | Standard CL vs CL Extension |
| Caching | Binary caching for fast re-runs |
| Configuration | JSON-based kernel and I/O setup |

**Speaker Notes**:
Recap the key points covered in this presentation. The framework enables kernel developers to focus on optimizations without writing host code.

---

### Slide 19: Q&A
**Title**: Questions & Discussion

**Content**:
- Questions about the framework?
- Next: Part II - Two-Phase Architecture (Compile/Execute Split)

**Speaker Notes**:
Open the floor for questions. Mention that Part II will cover the two-phase architecture for cross-platform deployment.

---
