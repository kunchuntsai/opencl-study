# Adding New Algorithms

Quick guide on the mandatory configuration and API requirements for adding algorithms.

## Table of Contents

- [Three Required Components](#three-required-components)
- [1. Configuration File (`config/<algo>.ini`)](#1-configuration-file-configalgoini)
  - [Basic Structure](#basic-structure)
  - [Configuration Parameters](#configuration-parameters)
  - [Custom Buffers](#custom-buffers)
- [2. Mandatory C API Functions](#2-mandatory-c-api-functions)
  - [Function 1: Reference Implementation](#function-1-reference-implementation)
  - [Function 2: Verification Function](#function-2-verification-function)
  - [Function 3: Kernel Arguments Setter](#function-3-kernel-arguments-setter)
- [3. Auto-Registration System](#3-auto-registration-system)
  - [How It Works](#how-it-works)
  - [After Adding Your Algorithm](#after-adding-your-algorithm)
- [Quick Reference](#quick-reference)
- [Directory Structure Example](#directory-structure-example)
- [Build and Run](#build-and-run)

---

## Three Required Components

To add a new algorithm (e.g., `erode3x3`):

1. **Configuration file:** `config/erode3x3.ini`
2. **C reference file:** `src/erode/c_ref/erode3x3_ref.c` with three mandatory functions
3. **OpenCL kernel:** `src/erode/cl/erode0.cl`

---

## 1. Configuration File (`config/<algo>.ini`)

### Basic Structure

```ini
# Algorithm ID is auto-derived from filename
# erode3x3.ini → op_id = "erode3x3"

[image]
input = test_data/erode3x3/input.bin
output = test_data/erode3x3/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920    # Can differ from src for resize operations
dst_height = 1080

# Kernel variant 0 (required)
[kernel.v0]
kernel_file = src/erode/cl/erode0.cl
kernel_function = erode3x3
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

# Optional: Additional variants
[kernel.v1]
kernel_file = src/erode/cl/erode1.cl
kernel_function = erode3x3_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

### Configuration Parameters

#### `[image]` Section (Required)
| Parameter | Description | Example |
|-----------|-------------|---------|
| `input` | Input image path | `test_data/erode3x3/input.bin` |
| `output` | Output image path | `test_data/erode3x3/output.bin` |
| `src_width` | Source width in pixels | `1920` |
| `src_height` | Source height in pixels | `1080` |
| `dst_width` | Destination width (optional, defaults to src_width) | `1920` |
| `dst_height` | Destination height (optional, defaults to src_height) | `1080` |

#### `[kernel.vN]` Sections (At least v0 required)
| Parameter | Description | Example |
|-----------|-------------|---------|
| `kernel_file` | Path to .cl file | `src/erode/cl/erode0.cl` |
| `kernel_function` | Kernel function name | `erode3x3` |
| `work_dim` | Work dimensions (1, 2, or 3) | `2` |
| `global_work_size` | Global work size (comma-separated) | `1920,1088` |
| `local_work_size` | Local work size (comma-separated) | `16,16` |

### Custom Buffers

Add `[buffer.<name>]` sections for additional buffers beyond input/output:

```ini
# Empty buffer for intermediate results
[buffer.tmp_global]
type = READ_WRITE
size_bytes = 314572800

# Buffer loaded from file (e.g., kernel weights)
[buffer.kernel_x]
type = READ_ONLY
data_type = float          # float, int, uchar, etc.
num_elements = 5
source_file = test_data/gaussian5x5/kernel_x.bin

# Another weights buffer
[buffer.kernel_y]
type = READ_ONLY
data_type = float
num_elements = 5
source_file = test_data/gaussian5x5/kernel_y.bin
```

**Buffer Types:**
- `READ_ONLY` - Kernel only reads (e.g., constant weights)
- `WRITE_ONLY` - Kernel only writes (e.g., output buffer)
- `READ_WRITE` - Kernel reads and writes (e.g., intermediate results)

**Buffer Allocation:**
- **Size-based:** Specify `size_bytes` for empty allocation
- **Data-based:** Specify `data_type`, `num_elements`, and `source_file` to load from file
- Custom buffers are indexed in order: first `[buffer.*]` section = index 0, etc.

---

## 2. Mandatory C API Functions

Every algorithm must implement these three functions in `src/<algo>/c_ref/<algo>_ref.c`:

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

### Function 3: Kernel Arguments Setter
```c
int <algo>_set_kernel_args(cl_kernel kernel,
                           cl_mem input_buf,
                           cl_mem output_buf,
                           const OpParams* params);
```

**Purpose:** Set all kernel arguments in exact order matching kernel signature

**Critical:** Argument order must **exactly match** your OpenCL kernel signature!

**Returns:**
- `0` on success
- `-1` on error

**Example for simple kernel:**
```c
// Kernel: __kernel void erode3x3(__global uchar* input,
//                                __global uchar* output,
//                                int width, int height)

int erode3x3_set_kernel_args(cl_kernel kernel, cl_mem input_buf,
                             cl_mem output_buf, const OpParams* params) {
    if (!kernel || !params) return -1;

    // arg 0: input
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
    // arg 1: output
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
    // arg 2: width
    clSetKernelArg(kernel, 2, sizeof(int), &params->src_width);
    // arg 3: height
    clSetKernelArg(kernel, 3, sizeof(int), &params->src_height);

    return 0;
}
```

**Accessing custom buffers:**
```c
// Kernel: __kernel void gaussian5x5(__global uchar* input,
//                                    __global uchar* output,
//                                    __global uchar* tmp_buffer,    // custom[0]
//                                    int width, int height,
//                                    __global float* kernel_x,      // custom[1]
//                                    __global float* kernel_y)      // custom[2]

// These types are defined in src/algorithm_runner.h
typedef struct {
    cl_mem buffer;
    unsigned char* host_data;
} RuntimeBuffer;

typedef struct {
    RuntimeBuffer buffers[MAX_CUSTOM_BUFFERS]; // MAX: 8
    int count;
} CustomBuffers;

int gaussian5x5_set_kernel_args(cl_kernel kernel, cl_mem input_buf,
                                cl_mem output_buf, const OpParams* params) {
    if (!kernel || !params || !params->custom_buffers) return -1;

    CustomBuffers* custom = params->custom_buffers;
    if (custom->count != 3) return -1;  // Expect 3 custom buffers

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &custom->buffers[0].buffer);  // tmp
    clSetKernelArg(kernel, 3, sizeof(int), &params->src_width);
    clSetKernelArg(kernel, 4, sizeof(int), &params->src_height);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &custom->buffers[1].buffer);  // kernel_x
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &custom->buffers[2].buffer);  // kernel_y

    return 0;
}
```

**Key points:**
- Custom buffers accessed via `params->custom_buffers`
- Type-safe access (no casting required)
- Buffers indexed in order they appear in `.ini` file
- First `[buffer.*]` → `custom->buffers[0]`, second → `custom->buffers[1]`, etc.

### Adapting Arguments Based on Host Type

If your algorithm has multiple kernel variants with different signatures, use `params->host_type`:

```c
int my_algo_set_kernel_args(cl_kernel kernel, cl_mem input_buf,
                            cl_mem output_buf, const OpParams* params) {
    if (!kernel || !params) return -1;

    cl_uint arg_idx = 0;

    // Standard arguments for all variants
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf);

    // Variant-specific arguments based on host_type
    if (params->host_type == HOST_TYPE_STANDARD) {
        // Standard variant: simple arguments
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width);
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height);
    } else if (params->host_type == HOST_TYPE_CL_EXTENSION) {
        // Extension variant: uses local memory, needs local_size argument
        size_t local_size = 16 * 16 * sizeof(float);
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width);
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height);
        clSetKernelArg(kernel, arg_idx++, local_size, NULL);  // Local memory
    }

    return 0;
}
```

**Use cases:**
- Different kernel signatures for standard vs. optimized variants
- Local memory allocation that varies by variant
- Different buffer configurations per variant

### Using Buffer Metadata

Each `RuntimeBuffer` in `params->custom_buffers` contains configuration metadata:

```c
typedef struct {
    cl_mem buffer;          // OpenCL buffer handle
    unsigned char* host_data; // Host data (NULL for empty buffers)
    BufferType type;        // READ_ONLY, WRITE_ONLY, READ_WRITE
    size_t size_bytes;      // Buffer size in bytes
} RuntimeBuffer;
```

**Example: Dynamic local memory allocation based on buffer size:**

```c
int my_algo_set_kernel_args(cl_kernel kernel, cl_mem input_buf,
                            cl_mem output_buf, const OpParams* params) {
    if (!kernel || !params || !params->custom_buffers) return -1;

    CustomBuffers* custom = params->custom_buffers;
    cl_uint arg_idx = 0;

    // Standard arguments
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf);
    clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width);
    clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height);

    // Custom buffer argument
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[0].buffer);

    // Allocate local memory based on buffer size
    if (params->host_type == HOST_TYPE_CL_EXTENSION) {
        // Use buffer size to determine local memory allocation
        size_t local_size = custom->buffers[0].size_bytes;
        clSetKernelArg(kernel, arg_idx++, local_size, NULL);  // Local memory
    }

    return 0;
}
```

**Use cases:**
- Dynamic local memory sizing based on buffer configuration
- Conditional argument setting based on buffer type (READ_ONLY vs READ_WRITE)
- Validation: ensure buffer size meets algorithm requirements
- Debugging: check buffer sizes match expected values

---

## 3. Auto-Registration System

**TL;DR:** Registration is automatic - just run `./scripts/generate_registry.sh` after adding your algorithm.

### How It Works

The script `scripts/generate_registry.sh` automatically:
1. Scans `src/` for algorithm reference files (`src/*/c_ref/*_ref.c`)
2. Extracts algorithm names from filenames
3. Generates `src/utils/auto_registry.c` with all registrations

**Generated code:**
- Declares `extern` functions for all three required functions
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
- This ID must match your `.ini` filename: `config/erode3x3.ini`

---

## Quick Reference

### Mandatory Files

| File | Purpose |
|------|---------|
| `config/<algo>.ini` | Algorithm configuration |
| `src/<algo>/c_ref/<algo>_ref.c` | Three required functions |
| `src/<algo>/cl/<algo>0.cl` | OpenCL kernel variant 0 |

### Mandatory Functions

| Function | Signature | Purpose |
|----------|-----------|---------|
| `<algo>_ref` | `void(const OpParams*)` | CPU reference implementation |
| `<algo>_verify` | `int(const OpParams*, float*)` | Verify GPU vs CPU |
| `<algo>_set_kernel_args` | `int(cl_kernel, cl_mem, cl_mem, const OpParams*)` | Set kernel arguments |

### Configuration Checklist

- [ ] Created `config/<algo>.ini` with `[image]` and `[kernel.v0]` sections
- [ ] Created `src/<algo>/c_ref/<algo>_ref.c` with three required functions
- [ ] Created `src/<algo>/cl/<algo>0.cl` kernel file
- [ ] Kernel signature matches `set_kernel_args()` exactly
- [ ] Algorithm filename matches `.ini` filename (e.g., `erode3x3_ref.c` ↔ `erode3x3.ini`)
- [ ] Run `./scripts/generate_registry.sh` to auto-register
- [ ] Built and tested successfully

---

## Directory Structure Example

```
config/
  └── erode3x3.ini              # Configuration

src/erode/
  ├── c_ref/
  │   └── erode3x3_ref.c        # Three required functions
  └── cl/
      ├── erode0.cl             # Variant 0 kernel
      └── erode1.cl             # Variant 1 kernel (optional)

test_data/erode3x3/
  ├── input.bin                 # Input image
  ├── output.bin                # Output image (generated)
  ├── golden/
  │   └── erode3x3.bin          # Cached CPU reference (auto-generated)
  └── kernels/
      └── erode0.bin            # Cached compiled kernel (auto-generated)
```

---

## Build and Run

```bash
# 1. Generate registry and build
# (scans src/ and auto-registers algorithms) from scripts/generate_registry.sh
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

- **[ALGORITHM_INTERFACE.md](ALGORITHM_INTERFACE.md)** - Detailed OpParams interface
- **[CONFIG_SYSTEM.md](CONFIG_SYSTEM.md)** - Configuration file format
- **[../ARCHITECTURE.md](../ARCHITECTURE.md)** - System architecture
- **[../README.md](../README.md)** - Main project documentation
