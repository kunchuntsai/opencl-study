# Adding New Algorithms

Quick guide on the mandatory configuration and API requirements for adding algorithms.

## Table of Contents

- [Three Required Components](#three-required-components)
- [1. Configuration File (`config/<algo>.json`)](#1-configuration-file-configalgojson)
  - [Basic Structure](#basic-structure)
  - [Configuration Parameters](#configuration-parameters)
  - [Custom Buffers](#custom-buffers)
- [2. Mandatory C API Functions](#2-mandatory-c-api-functions)
  - [Function 1: Reference Implementation](#function-1-reference-implementation)
  - [Function 2: Verification Function](#function-2-verification-function)
- [3. Auto-Registration System](#3-auto-registration-system)
  - [How It Works](#how-it-works)
  - [After Adding Your Algorithm](#after-adding-your-algorithm)
- [Quick Reference](#quick-reference)
- [Directory Structure Example](#directory-structure-example)
- [Build and Run](#build-and-run)

---

## Three Required Components

To add a new algorithm (e.g., `erode3x3`):

1. **Configuration file:** `config/erode3x3.json`
2. **C reference file:** `examples/erode3x3/c_ref/erode3x3_ref.c` with two mandatory functions
3. **OpenCL kernel:** `examples/erode3x3/cl/erode0.cl`

---

## 1. Configuration File (`config/<algo>.json`)

### Basic Structure

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
            "kernel_file": "examples/erode3x3/cl/erode0.cl",
            "kernel_function": "erode3x3",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]}
            ]
        },
        "v1": {
            "host_type": "cl_extension",
            "kernel_file": "examples/erode3x3/cl/erode1.cl",
            "kernel_function": "erode3x3_optimized",
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

**Note:** The `op_id` is automatically derived from the config filename:
- `erode3x3.json` → op_id = `"erode3x3"`

### Configuration Parameters

#### Input/Output Section (Required)

References images defined in `config/inputs.json` and `config/outputs.json`:

```json
"input": {
    "input_image_id": "image_1"
},
"output": {
    "output_image_id": "output_1"
}
```

#### Verification Section

| Parameter | Type | Description | Example |
|-----------|------|-------------|---------|
| `tolerance` | float | Max per-pixel difference allowed | `0` (exact), `1` (±1) |
| `error_rate_threshold` | float | Max fraction of pixels that can exceed tolerance | `0.001` (0.1%) |
| `golden_source` | string | Source of golden sample: `c_ref` or `file` | `"c_ref"` |
| `golden_file` | string | Path to golden file (when `golden_source` is `file`) | `"test_data/algo/golden.bin"` |

#### Kernels Section (At least v0 required)

**Note:** The variant number in `"v0"`, `"v1"`, etc. is automatically used as the `kernel_variant` value in `params->kernel_variant`. No need to specify it manually!

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `kernel_file` | string | Yes | Path to .cl file |
| `kernel_function` | string | Yes | Kernel function name |
| `work_dim` | int | Yes | Work dimensions (1, 2, or 3) |
| `global_work_size` | array | Yes | Global work size per dimension |
| `local_work_size` | array | Yes | Local work group size per dimension |
| `host_type` | string | No | `standard` (default) or `cl_extension` |
| `kernel_option` | string | No | Compiler options (e.g., `-cl-fast-relaxed-math`) |
| `kernel_args` | array | Yes | Kernel argument definitions |

#### Kernel Arguments (kernel_args)

| Key | Format | Description | Example |
|-----|--------|-------------|---------|
| `i_buffer` | `["type", "name"]` | Input buffer | `{"i_buffer": ["uchar", "src"]}` |
| `o_buffer` | `["type", "name"]` | Output buffer | `{"o_buffer": ["uchar", "dst"]}` |
| `buffer` | `["type", "name", size]` | Custom buffer with size | `{"buffer": ["uchar", "tmp", 45000]}` |
| `param` | `["type", "name"]` | Scalar parameter | `{"param": ["int", "src_width"]}` |

**Supported data types:**
- Buffers: `uchar`, `float`, `int`, `short`
- Params: `int`, `float`, `size_t`

### Custom Buffers

Add a `buffers` section for additional buffers beyond input/output:

```json
"buffers": {
    "tmp_global": {
        "type": "READ_WRITE",
        "size_bytes": 400000
    },
    "kernel_x": {
        "type": "READ_ONLY",
        "data_type": "float",
        "num_elements": 5,
        "source_file": "test_data/gaussian5x5/kernel_x.bin"
    },
    "kernel_y": {
        "type": "READ_ONLY",
        "data_type": "float",
        "num_elements": 5,
        "source_file": "test_data/gaussian5x5/kernel_y.bin"
    }
}
```

**Buffer Types:**
- `READ_ONLY` - Kernel only reads (e.g., constant weights)
- `WRITE_ONLY` - Kernel only writes (e.g., output buffer)
- `READ_WRITE` - Kernel reads and writes (e.g., intermediate results)

**Buffer Allocation:**
- **Size-based:** Specify `size_bytes` for empty allocation (supports expressions like `"1920 * 1080"`)
- **Data-based:** Specify `data_type`, `num_elements`, and `source_file` to load from file

### Scalars Section

Define scalar parameters passed to kernels:

```json
"scalars": {
    "sigma": {
        "type": "float",
        "value": 1.4
    },
    "kernel_radius": {
        "type": "int",
        "value": 2
    },
    "buffer_size": {
        "type": "size_t",
        "value": "1920 * 1080 * 4"
    }
}
```

Supported types: `int`, `float`, `size_t`

---

## 2. Mandatory C API Functions

Every algorithm must implement these two functions in `examples/<algo>/c_ref/<algo>_ref.c`:

### Function 1: Reference Implementation
```c
void <algo>_ref(const OpParams* params);
```

**Purpose:** CPU implementation serving as ground truth for verification

**Parameters available in `OpParams*`:**
- `params->input` - Input image buffer
- `params->output` - Output image buffer
- `params->src_width`, `params->src_height` - Source dimensions
- `params->dst_width`, `params->dst_height` - Destination dimensions
- `params->border_mode`, `params->border_value` - Border handling
- `params->custom_buffers` - Custom buffer collection (NULL if none)

**Must:**
- Validate `params != NULL`
- Implement the algorithm correctly (this is the golden reference!)
- Write results to `params->output`

---

### Function 2: Verification Function
```c
int <algo>_verify(const OpParams* params, float* max_error);
```

**Purpose:** Compare GPU output against CPU reference

**Returns:**
- `1` if verification passed
- `0` if verification failed

**Parameters:**
- `params->ref_output` - CPU reference output
- `params->gpu_output` - GPU kernel output
- `params->dst_width`, `params->dst_height` - Output dimensions
- `max_error` - (output) Maximum error found

**Use provided helpers:**
```c
// For operations requiring exact match (morphology, etc.)
return verify_exact_match(params->gpu_output, params->ref_output,
                         params->dst_width, params->dst_height, max_error);

// For operations with rounding tolerance (filters, etc.)
return verify_with_tolerance(params->gpu_output, params->ref_output,
                            params->dst_width, params->dst_height,
                            1.0f,    // max pixel difference
                            0.001f,  // max fraction of pixels that can differ
                            max_error);
```

---

## 3. Auto-Registration System

**TL;DR:** Registration is automatic - just run `./scripts/generate_registry.sh` after adding your algorithm.

### How It Works

The script `scripts/generate_registry.sh` automatically:
1. Scans `examples/` for algorithm reference files (`examples/*/c_ref/*_ref.c`)
2. Extracts algorithm names from filenames
3. Generates `src/core/auto_registry.c` with all registrations

**Generated code:**
- Declares `extern` functions for the two required functions
- Calls `register_algorithm()` for each algorithm in `register_all_algorithms()`
- Uses constructor attribute to run before `main()`

### After Adding Your Algorithm

Simply regenerate the registry:

```bash
./scripts/generate_registry.sh
./scripts/build.sh
```

**Important:**
- Algorithm ID is derived from filename: `erode3x3_ref.c` → `"erode3x3"`
- This ID must match your `.json` filename: `config/erode3x3.json`

---

## Quick Reference

### Mandatory Files

| File | Purpose |
|------|---------|
| `config/<algo>.json` | Algorithm configuration (JSON format) |
| `examples/<algo>/c_ref/<algo>_ref.c` | Two required functions |
| `examples/<algo>/cl/<algo>0.cl` | OpenCL kernel variant 0 |

### Mandatory Functions

| Function | Signature | Purpose |
|----------|-----------|---------|
| `<algo>_ref` | `void(const OpParams*)` | CPU reference implementation |
| `<algo>_verify` | `int(const OpParams*, float*)` | Verify GPU vs CPU |

**Note:** Kernel arguments are now fully config-driven via `kernel_args` in JSON. No custom `set_kernel_args` function required!

### Configuration Checklist

- [ ] Created `config/<algo>.json` with `input`, `output`, and `kernels` sections
- [ ] Created `examples/<algo>/c_ref/<algo>_ref.c` with two required functions
- [ ] Created `examples/<algo>/cl/<algo>0.cl` kernel file
- [ ] Kernel signature matches `kernel_args` order exactly
- [ ] Algorithm filename matches `.json` filename (e.g., `erode3x3_ref.c` ↔ `erode3x3.json`)
- [ ] Run `./scripts/generate_registry.sh` to auto-register
- [ ] Built and tested successfully

---

## Directory Structure Example

```
config/
  └── erode3x3.json               # Configuration (JSON format)

examples/erode3x3/
  ├── c_ref/
  │   └── erode3x3_ref.c          # Two required functions
  └── cl/
      ├── erode0.cl               # Variant 0 kernel
      └── erode1.cl               # Variant 1 kernel (optional)

test_data/erode3x3/
  ├── input.bin                   # Input image
  ├── output.bin                  # Output image (generated)
  ├── golden/
  │   └── erode3x3.bin            # Cached CPU reference (auto-generated)
  └── kernels/
      └── erode0.bin              # Cached compiled kernel (auto-generated)
```

---

## Build and Run

```bash
# 1. Generate registry and build
# (scans examples/ and auto-registers algorithms) from scripts/generate_registry.sh
./scripts/build.sh

# 2. List available algorithms
./build/opencl_host

# 3. Run algorithm variant 0
./build/opencl_host erode3x3 0

# 4. Run algorithm variant 1
./build/opencl_host erode3x3 1
```

**Note:** Only need to run `generate_registry.sh` when adding/removing algorithms, not for code changes to existing algorithms.

---

## Related Documentation

- **[CONFIG_SYSTEM.md](CONFIG_SYSTEM.md)** - Configuration file format (JSON)
- **[CONFIG_KERNEL_ARG.md](CONFIG_KERNEL_ARG.md)** - Kernel argument configuration
- **[../ARCHITECTURE.md](../ARCHITECTURE.md)** - System architecture
- **[../README.md](../README.md)** - Main project documentation
