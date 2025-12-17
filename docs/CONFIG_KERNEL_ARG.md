# Config-Driven Kernel Argument Packing

## Overview

The framework automatically sets OpenCL kernel arguments based on configuration in `.json` files. This eliminates the need to write custom `SetKernelArgs` functions for each algorithm.

## Key Features

- **Fully config-driven**: Specify kernel arguments in `.json` files using descriptive key format
- **Explicit argument ordering**: All arguments including input/output are defined in `kernel_args` array
- **Flexible buffer references**: Reference buffers by name
- **Size_t support**: Pass buffer sizes to kernels using `param` with `size_t` type
- **Variant-specific args**: Each kernel variant can specify different argument orders

## How It Works

### 1. Argument Types

The following argument types are supported:

| Key | Format | Description | Example |
|-----|--------|-------------|---------|
| `i_buffer` | `{"i_buffer": ["type", "name"]}` | Input buffer | `{"i_buffer": ["uchar", "src"]}` |
| `o_buffer` | `{"o_buffer": ["type", "name"]}` | Output buffer | `{"o_buffer": ["uchar", "dst"]}` |
| `buffer` | `{"buffer": ["type", "name"]}` | Custom buffer reference | `{"buffer": ["float", "kernel_x"]}` |
| `param` | `{"param": ["type", "name"]}` | Scalar parameter from OpParams | `{"param": ["int", "src_width"]}` |

### 2. Supported Data Types

For buffers (`i_buffer`, `o_buffer`, `buffer`):
- `uchar` - 8-bit unsigned char
- `float` - 32-bit floating point
- `int` - 32-bit signed integer
- `short` - 16-bit signed integer

For params (`param`):
- `int` - Integer parameter
- `float` - Float parameter
- `size_t` - Size value (for buffer sizes, etc.)

### 3. Argument Order

Arguments are set in the order they appear in the `kernel_args` array. This gives full control over the kernel signature:

```json
"kernel_args": [
    {"i_buffer": ["uchar", "src"]},      // arg 0
    {"o_buffer": ["uchar", "dst"]},      // arg 1
    {"param": ["int", "src_width"]},     // arg 2
    {"param": ["int", "src_height"]}     // arg 3
]
```

## Configuration Examples

### Example 1: Simple Algorithm (Dilate 3x3)

**Config file**: `config/dilate3x3.json`
```json
{
    "input": {
        "input_image_id": "image_1"
    },
    "output": {
        "output_image_id": "output_1"
    },
    "kernels": {
        "v0": {
            "kernel_file": "examples/dilate3x3/cl/dilate0.cl",
            "kernel_function": "dilate3x3",
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
            "kernel_file": "examples/dilate3x3/cl/dilate1.cl",
            "kernel_function": "dilate3x3_optimized",
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

**Resulting kernel signature**:
```c
__kernel void dilate3x3(__global const uchar* src,    // arg 0 (i_buffer)
                        __global uchar* dst,           // arg 1 (o_buffer)
                        int src_width,                 // arg 2 (param)
                        int src_height)                // arg 3 (param)
```

### Example 2: Algorithm with Custom Buffers (Gaussian 5x5)

**Config file**: `config/gaussian5x5.json`
```json
{
    "input": {
        "input_image_id": "image_2"
    },
    "output": {
        "output_image_id": "output_2"
    },
    "buffers": {
        "tmp_global": {
            "type": "READ_WRITE",
            "size_bytes": 400000
        },
        "tmp_global2": {
            "type": "READ_WRITE",
            "size_bytes": 512000
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
    },
    "kernels": {
        "v0": {
            "host_type": "standard",
            "kernel_file": "examples/gaussian5x5/cl/gaussian0.cl",
            "kernel_function": "gaussian5x5",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]},
                {"buffer": ["float", "kernel_x"]},
                {"buffer": ["float", "kernel_y"]}
            ]
        },
        "v1": {
            "host_type": "cl_extension",
            "kernel_file": "examples/gaussian5x5/cl/gaussian1.cl",
            "kernel_function": "gaussian5x5_optimized",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"buffer": ["uchar", "tmp_global"]},
                {"param": ["size_t", "tmp_global_size"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]},
                {"buffer": ["float", "kernel_x"]},
                {"buffer": ["float", "kernel_y"]}
            ]
        },
        "v2": {
            "host_type": "cl_extension",
            "kernel_file": "examples/gaussian5x5/cl/gaussian2.cl",
            "kernel_function": "gaussian5x5_horizontal",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"buffer": ["uchar", "tmp_global"]},
                {"param": ["size_t", "tmp_global_size"]},
                {"buffer": ["uchar", "tmp_global2"]},
                {"param": ["size_t", "tmp_global2_size"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]},
                {"buffer": ["float", "kernel_x"]},
                {"buffer": ["float", "kernel_y"]}
            ]
        }
    }
}
```

**Resulting kernel signatures**:

**Variant 0**:
```c
__kernel void gaussian5x5(__global const uchar* src,      // arg 0 (i_buffer)
                          __global uchar* dst,             // arg 1 (o_buffer)
                          int src_width,                   // arg 2 (param)
                          int src_height,                  // arg 3 (param)
                          __global const float* kernel_x,  // arg 4 (buffer)
                          __global const float* kernel_y)  // arg 5 (buffer)
```

**Variant 1**:
```c
__kernel void gaussian5x5_optimized(__global const uchar* src,      // arg 0 (i_buffer)
                                    __global uchar* dst,             // arg 1 (o_buffer)
                                    __global uchar* tmp_buffer,      // arg 2 (buffer)
                                    unsigned long tmp_size,          // arg 3 (param size_t)
                                    int src_width,                   // arg 4 (param)
                                    int src_height,                  // arg 5 (param)
                                    __global const float* kernel_x,  // arg 6 (buffer)
                                    __global const float* kernel_y)  // arg 7 (buffer)
```

**Variant 2**:
```c
__kernel void gaussian5x5_horizontal(__global const uchar* src,       // arg 0 (i_buffer)
                                     __global uchar* dst,              // arg 1 (o_buffer)
                                     __global uchar* tmp_buffer,       // arg 2 (buffer)
                                     unsigned long tmp_size,           // arg 3 (param size_t)
                                     __global uchar* tmp_buffer2,      // arg 4 (buffer)
                                     unsigned long tmp_size2,          // arg 5 (param size_t)
                                     int src_width,                    // arg 6 (param)
                                     int src_height,                   // arg 7 (param)
                                     __global const float* kernel_x,   // arg 8 (buffer)
                                     __global const float* kernel_y)   // arg 9 (buffer)
```

## Available OpParams Fields

The following fields can be referenced in `param`:

**Integer fields** (`{"param": ["int", "field_name"]}`):
- `src_width` - Source image width
- `src_height` - Source image height
- `src_stride` - Source image stride (bytes per row)
- `src_channels` - Source image channels
- `dst_width` - Destination image width
- `dst_height` - Destination image height
- `dst_stride` - Destination image stride
- `dst_channels` - Destination image channels

**Float fields** (`{"param": ["float", "field_name"]}`):
- Custom scalar values defined in `scalars` section

**Size fields** (`{"param": ["size_t", "field_name"]}`):
- Buffer size values (e.g., `tmp_global_size`)

## Best Practices

1. **Use descriptive buffer names**: Clear names improve readability:
   ```json
   {"buffer": ["float", "horizontal_weights"]}
   ```

2. **Match OpenCL kernel signatures exactly**: The order in `kernel_args` must match your kernel function parameters.

3. **Test each variant**: Ensure all variants pass verification to confirm correct argument order.

4. **Group related arguments**: Keep input buffers first, output next, then parameters:
   ```json
   "kernel_args": [
       {"i_buffer": ["uchar", "src"]},
       {"o_buffer": ["uchar", "dst"]},
       {"buffer": ["float", "weights"]},
       {"param": ["int", "src_width"]},
       {"param": ["int", "src_height"]}
   ]
   ```

## Implementation Details

### Code Structure

- **Config parsing**: `src/utils/config.c::ParseKernelArgsJson()`
- **Argument setting**: `src/platform/opencl_utils.c::OpenclSetKernelArgs()`
- **Data structures**: `src/utils/config.h::KernelArgDescriptor`

### Argument Types Enum

```c
typedef enum {
    KERNEL_ARG_TYPE_NONE = 0,
    KERNEL_ARG_TYPE_BUFFER_INPUT,   // i_buffer
    KERNEL_ARG_TYPE_BUFFER_OUTPUT,  // o_buffer
    KERNEL_ARG_TYPE_BUFFER_CUSTOM,  // buffer
    KERNEL_ARG_TYPE_SCALAR_INT,     // param with int
    KERNEL_ARG_TYPE_SCALAR_FLOAT,   // param with float
    KERNEL_ARG_TYPE_SCALAR_SIZE,    // param with size_t
} KernelArgType;
```

### KernelArgDescriptor Structure

```c
typedef struct {
    KernelArgType arg_type;  // Type of argument (buffer or scalar)
    DataType data_type;      // Data type (uchar, int, float, etc.)
    char source_name[64];    // Name of the argument
} KernelArgDescriptor;
```

## Limitations

- Maximum 32 kernel arguments per variant (`MAX_KERNEL_ARGS`)
- Buffer names must match those defined in `buffers` section
- All arguments must be explicitly specified in `kernel_args`

## Migration from Old Format

### Old Format (deprecated)
```json
{"type": "input", "source": "input_image_id"}
{"type": "int", "source": "src_width"}
{"type": "buffer", "source": "kernel_x"}
```

### New Format
```json
{"i_buffer": ["uchar", "src"]}
{"param": ["int", "src_width"]}
{"buffer": ["float", "kernel_x"]}
```

The new format is more explicit about data types and uses descriptive keys that clearly indicate the argument category.
