# Adding New Algorithms

Quick guide on the mandatory configuration and API requirements for adding algorithms.

## Table of Contents

- [Three Required Components](#three-required-components)
- [Quick Start with Script](#quick-start-with-script)
- [1. Configuration File (`config/<algo>.json`)](#1-configuration-file-configalgojson)
  - [Basic Structure](#basic-structure)
  - [Configuration Parameters](#configuration-parameters)
  - [Custom Buffers](#custom-buffers)
- [2. Mandatory C API Function](#2-mandatory-c-api-function)
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
2. **C reference file:** `examples/erode3x3/c_ref/erode3x3_ref.c` with reference function
3. **OpenCL kernel:** `examples/erode3x3/cl/erode3x30.cl`

---

## Quick Start with Script

Use the helper script to create all boilerplate:

```bash
./scripts/create_new_algo.sh erode3x3
```

This creates:
- `config/erode3x3.json` - JSON configuration
- `examples/erode3x3/c_ref/erode3x3_ref.c` - C reference implementation
- `examples/erode3x3/cl/erode3x30.cl` - OpenCL kernel template

Then add input/output entries to `config/inputs.json` and `config/outputs.json`.

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

**Note:** The variant number in `"v0"`, `"v1"`, etc. determines the selection index (e.g., `v0` → select with `0`, `v1` → select with `1`).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `description` | string | No | Human-readable description shown in variant menu |
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
| `struct` | `["field1", "field2", ...]` | Packed struct from scalars | `{"struct": ["f1", "f2"]}` |

**Supported data types:**
- Buffers: `uchar`, `float`, `int`, `short`
- Params: `int`, `float`, `size_t`
- Struct fields: References scalars defined in `scalars` section

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

## 2. Mandatory C API Function

Every algorithm must implement one function in `examples/<algo>/c_ref/<algo>_ref.c`:

### Reference Implementation
```c
void <AlgoName>Ref(const OpParams* params);
```

**Note:** Function name uses PascalCase (e.g., `Erode3x3Ref`, `Gaussian5x5Ref`).

**Purpose:** CPU implementation serving as ground truth for verification

**Parameters available in `OpParams*`:**
- `params->input` - Input image buffer
- `params->output` - Output image buffer
- `params->src_width`, `params->src_height` - Source dimensions
- `params->dst_width`, `params->dst_height` - Destination dimensions
- `params->custom_buffers` - Custom buffer collection (NULL if none)
- `params->custom_scalars` - Custom scalar values (NULL if none)

**Must:**
- Validate `params != NULL`
- Implement the algorithm correctly (this is the golden reference!)
- Write results to `params->output`

**Verification** is handled automatically based on the `verification` section in your JSON config:
- `tolerance` - Max per-pixel difference allowed
- `error_rate_threshold` - Max fraction of pixels that can exceed tolerance
- `golden_source` - Use `"c_ref"` to use reference function, or `"file"` to use a golden file

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
| `examples/<algo>/c_ref/<algo>_ref.c` | Reference implementation function |
| `examples/<algo>/cl/<algo>0.cl` | OpenCL kernel variant 0 |

### Mandatory Function

| Function | Signature | Purpose |
|----------|-----------|---------|
| `<AlgoName>Ref` | `void(const OpParams*)` | CPU reference implementation |

**Notes:**
- Function name uses PascalCase (e.g., `Erode3x3Ref`)
- Verification is config-driven via `tolerance` and `error_rate_threshold` in JSON
- Kernel arguments are config-driven via `kernel_args` in JSON

### Configuration Checklist

- [ ] Created `config/<algo>.json` with `input`, `output`, `verification`, and `kernels` sections
- [ ] Created `examples/<algo>/c_ref/<algo>_ref.c` with `<AlgoName>Ref()` function
- [ ] Created `examples/<algo>/cl/<algo>0.cl` kernel file
- [ ] Added input entry to `config/inputs.json`
- [ ] Added output entry to `config/outputs.json`
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
- **[../ARCHITECTURE.md](../ARCHITECTURE.md)** - System architecture
- **[../README.md](../README.md)** - Main project documentation
