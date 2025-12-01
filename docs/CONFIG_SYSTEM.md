# Per-Algorithm Configuration System

## Overview

The framework now uses per-algorithm configuration files for better maintainability and scalability. Each algorithm has its own `.ini` file containing algorithm-specific settings and kernel variants.

## Configuration File Structure

```
config/
├── dilate3x3.ini      # Dilate 3x3 specific config
├── gaussian5x5.ini    # Gaussian 5x5 specific config
└── myalgo.ini         # Your custom algorithm config
```

Each configuration file contains three types of sections:
- `[image]` - Input/output settings and image dimensions
- `[buffer.*]` - Custom buffer definitions (optional)
- `[kernel.v*]` - Kernel variant configurations

## Auto-Derivation of op_id

The `op_id` field is **automatically derived** from the config filename:

| Config File              | Auto-derived op_id | Registered Algorithm Required |
|--------------------------|--------------------|-----------------------------|
| `config/dilate3x3.ini`   | `dilate3x3`        | Yes - must have `dilate3x3` algorithm |
| `config/gaussian5x5.ini` | `gaussian5x5`      | Yes - must have `gaussian5x5` algorithm |
| `config/my_algo.ini`     | `my_algo`          | Yes - must have `my_algo` algorithm |

**Important:** The auto-derived `op_id` must match a registered algorithm in the system. You can manually specify `op_id` in the `[image]` section to override, but this is rarely needed.

## Configuration Parameters Reference

### [image] Section Parameters

| Parameter | Type | Required | Description | Example |
|-----------|------|----------|-------------|---------|
| `input` | string | Yes | Path to input image binary file | `test_data/gaussian5x5/input.bin` |
| `output` | string | Yes | Path to output image binary file | `test_data/gaussian5x5/output.bin` |
| `src_width` | integer | Yes | Source image width in pixels | `1920` |
| `src_height` | integer | Yes | Source image height in pixels | `1080` |
| `dst_width` | integer | Yes | Destination image width in pixels | `1920` |
| `dst_height` | integer | Yes | Destination image height in pixels | `1080` |
| `op_id` | string | No | Algorithm operation ID (auto-derived from filename if omitted) | `gaussian5x5` |

### [buffer.*] Section Parameters (Custom Buffers)

Custom buffers are defined with section name `[buffer.<name>]` where `<name>` is a descriptive identifier.

| Parameter | Type | Required | Description | Example |
|-----------|------|----------|-------------|---------|
| `type` | enum | Yes | Buffer memory type: `READ_ONLY`, `WRITE_ONLY`, or `READ_WRITE` | `READ_WRITE` |
| `size_bytes` | integer | Conditional* | Buffer size in bytes (for empty buffers) | `314572800` |
| `source_file` | string | Conditional* | Path to binary file to initialize buffer data (for file-backed buffers) | `test_data/gaussian5x5/kernel_x.bin` |
| `data_type` | string | Conditional* | Element data type: `float`, `int`, `char`, etc. (required with `source_file`) | `float` |
| `num_elements` | integer | Conditional* | Number of elements (required with `source_file`) | `5` |

\* **Buffer Types:**
- **Empty buffer**: Specify `type` + `size_bytes` (buffer allocated but not initialized)
- **File-backed buffer**: Specify `type` + `source_file` + `data_type` + `num_elements` (buffer loaded from file; `size_bytes` auto-calculated)

**Buffer Naming Examples:**
- `[buffer.tmp_global]` - Temporary global memory buffer (empty)
- `[buffer.kernel_x]` - Horizontal kernel weights (file-backed)
- `[buffer.kernel_y]` - Vertical kernel weights (file-backed)

### [kernel.v*] Section Parameters (Kernel Variants)

Kernel variants are defined with section name `[kernel.v<N>]` where `<N>` is the variant number (0, 1, 2, ...).

**Important:** The variant number in the section name (e.g., `v0`, `v1`, `v2`) is automatically used as the `kernel_variant` value passed to your `set_kernel_args()` function via `params->kernel_variant`. No manual configuration needed!

| Parameter | Type | Required | Description | Example |
|-----------|------|----------|-------------|---------|
| `kernel_file` | string | Yes | Path to OpenCL kernel source file | `src/gaussian/cl/gaussian0.cl` |
| `kernel_function` | string | Yes | Name of the kernel function to execute | `gaussian5x5` |
| `work_dim` | integer | Yes | Number of work dimensions (1, 2, or 3) | `2` |
| `global_work_size` | integers | Yes | Global work size (comma-separated for multi-dim) | `1920,1088` |
| `local_work_size` | integers | Yes | Local work group size (comma-separated for multi-dim) | `16,16` |
| `host_type` | enum | No | Host API type: `standard` (default) or `cl_extension` | `cl_extension` |

### Host Type Explanation

The `host_type` parameter determines which OpenCL host API is used to execute the kernel:

#### `standard` (Default)
- Uses the standard OpenCL API (`clSetKernelArg`, `clEnqueueNDRangeKernel`)
- Portable across all OpenCL implementations
- Requires manual buffer and argument management
- Best for: Initial implementations, cross-platform compatibility

#### `cl_extension`
- Uses the custom CL extension API framework
- Provides vendor-specific optimizations and simplified buffer management
- Automatically handles custom buffer setup from configuration
- Supports advanced features like zero-copy buffers and custom memory types
- Best for: Performance-optimized variants, vendor-specific tuning

**When to use each:**
- Start with `standard` for basic implementations
- Use `cl_extension` for optimized variants that need custom buffers or vendor-specific features
- Both can coexist in the same config file as different variants

### Kernel Variant Explanation

The `kernel_variant` value is **automatically derived** from the section name. When you define `[kernel.v0]`, `params->kernel_variant` is set to `0`. When you define `[kernel.v1]`, it's set to `1`, and so on.

**Purpose:**
- Passed to `set_kernel_args()` via `params->kernel_variant`
- Allows the same algorithm to support multiple kernel signatures
- Enables variant-specific argument handling logic
- **Automatically extracted from section name - no manual configuration needed!**

**Example use case:**

```ini
# Variant 0: Simple signature (input, output, width, height)
# kernel_variant will automatically be 0
[kernel.v0]
kernel_file = src/myalgo/cl/myalgo0.cl
kernel_function = myalgo_basic
...

# Variant 1: Extended signature with buffers (input, output, tmp_buf, tmp_size, width, height)
# kernel_variant will automatically be 1
[kernel.v1]
kernel_file = src/myalgo/cl/myalgo1.cl
kernel_function = myalgo_optimized
...

# Variant 2: Different signature (input, output, tmp_buf1, tmp_size1, tmp_buf2, tmp_size2, width, height)
# kernel_variant will automatically be 2
[kernel.v2]
kernel_file = src/myalgo/cl/myalgo2.cl
kernel_function = myalgo_twopass
...
```

In your `set_kernel_args()` function, you can check `params->kernel_variant` to set arguments correctly:

```c
int myalgo_set_kernel_args(cl_kernel kernel, cl_mem input_buf,
                           cl_mem output_buf, const OpParams* params) {
    cl_uint arg_idx = 0;

    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf);

    if (params->kernel_variant == 1) {
        // Variant 1: add tmp buffer and size
        clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[0].buffer);
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &custom->buffers[0].size_bytes);
    } else if (params->kernel_variant == 2) {
        // Variant 2: add two tmp buffers and sizes
        clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[0].buffer);
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &custom->buffers[0].size_bytes);
        clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[1].buffer);
        clSetKernelArg(kernel, arg_idx++, sizeof(int), &custom->buffers[1].size_bytes);
    }

    clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width);
    clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height);
    return 0;
}
```

**Note:** The variant number is automatically extracted from the section name. `[kernel.v0]` → variant 0, `[kernel.v1]` → variant 1, etc. This eliminates configuration errors and makes the intent clear from the section name itself.

## Configuration Best Practices

1. **One algorithm = One config file**: Name your config file after the algorithm (e.g., `dilate3x3.ini`)
2. **Omit op_id field**: The filename automatically becomes the op_id
3. **Use descriptive variant names**: Add comments explaining what each variant optimizes
4. **Group related variants**: Put all variants of an algorithm in its config file
5. **Start with `standard` host type**: Use `standard` for initial implementations, then add `cl_extension` variants for optimization

## Implementation Details

**Config path resolution:** `resolve_config_path()` in `src/utils/config.c` automatically resolves algorithm names to `config/<name>.ini`

**op_id extraction:** `extract_op_id_from_path()` in `src/utils/config.c` derives the op_id from the config filename
