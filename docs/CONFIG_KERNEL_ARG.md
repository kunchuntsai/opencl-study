# Config-Driven Kernel Argument Packing

## Overview

The framework automatically sets OpenCL kernel arguments based on configuration in `.ini` files. This eliminates the need to write custom `SetKernelArgs` functions for each algorithm.

## Key Features

- **Fully config-driven**: Specify kernel arguments in `.ini` files using `{type, source}` format
- **Implicit input/output**: Input and output buffers are always set as `arg0` and `arg1` automatically
- **Flexible buffer references**: Reference buffers by name or by array index
- **Size_t support**: Pass buffer sizes to kernels using `{size_t, buffer.size}` syntax
- **Variant-specific args**: Each kernel variant can specify different argument orders

## How It Works

### 1. Argument Types

The following argument types are supported:

| Type | Format | Description | Example |
|------|--------|-------------|---------|
| `buffer` | `{buffer, name}` or `{buffer, index}` | Custom buffer reference | `{buffer, kernel_x}` or `{buffer, 2}` |
| `int` | `{int, param_name}` | Integer parameter from OpParams | `{int, src_width}` |
| `size_t` | `{size_t, buffer.size}` or `{size_t, index.size}` | Buffer size as unsigned long | `{size_t, tmp_global.size}` or `{size_t, 0.size}` |

### 2. Argument Order

Arguments are set in this order:
1. **arg0**: Input buffer (implicit, always set automatically)
2. **arg1**: Output buffer (implicit, always set automatically)
3. **arg2+**: Arguments specified in `kernel_args` (in the order defined)

### 3. Buffer References

Buffers can be referenced in two ways:

#### By Name (Recommended for readability)
```ini
[buffer.tmp_global]
type = READ_WRITE
size_bytes = 400000

[buffer.kernel_x]
type = READ_ONLY
data_type = float
num_elements = 5
source_file = test_data/gaussian5x5/kernel_x.bin

[kernel.v1]
kernel_args = {buffer, tmp_global} {buffer, kernel_x}
```

#### By Index (Recommended when variants need different buffer orders)
Buffers are indexed in the order they appear in the config file:
```ini
[buffer.tmp_global]     # index 0
...
[buffer.tmp_global2]    # index 1
...
[buffer.kernel_x]       # index 2
...
[buffer.kernel_y]       # index 3
...

[kernel.v1]
kernel_args = {buffer, 0} {buffer, 2}  # Use tmp_global and kernel_x
```

## Configuration Examples

### Example 1: Simple Algorithm (Dilate 3x3)

**Config file**: `config/dilate3x3.ini`
```ini
[output]
dst_width = 1920
dst_height = 1080

# Variant 0: Basic implementation
[kernel.v0]
kernel_file = examples/dilate3x3/cl/dilate0.cl
kernel_function = dilate3x3
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
kernel_args = {int, src_width} {int, src_height}

# Variant 1: Optimized with local memory
[kernel.v1]
kernel_file = examples/dilate3x3/cl/dilate1.cl
kernel_function = dilate3x3_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
kernel_args = {int, src_width} {int, src_height}
```

**Resulting kernel signature**:
```c
__kernel void dilate3x3(__global const uchar* input,    // arg 0 (implicit)
                        __global uchar* output,          // arg 1 (implicit)
                        int width,                       // arg 2 (from kernel_args[0])
                        int height)                      // arg 3 (from kernel_args[1])
```

### Example 2: Algorithm with Custom Buffers (Gaussian 5x5)

**Config file**: `config/gaussian5x5.ini`
```ini
[output]
dst_width = 1920
dst_height = 1080

# Define custom buffers (indexed 0-3 in order of appearance)
[buffer.tmp_global]      # index 0
type = READ_WRITE
size_bytes = 400000

[buffer.tmp_global2]     # index 1
type = READ_WRITE
size_bytes = 512000

[buffer.kernel_x]        # index 2
type = READ_ONLY
data_type = float
num_elements = 5
source_file = test_data/gaussian5x5/kernel_x.bin

[buffer.kernel_y]        # index 3
type = READ_ONLY
data_type = float
num_elements = 5
source_file = test_data/gaussian5x5/kernel_y.bin

# Variant 0: Basic implementation (no tmp buffers)
[kernel.v0]
kernel_file = examples/gaussian5x5/cl/gaussian0.cl
kernel_function = gaussian5x5
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
kernel_args = {int, src_width} {int, src_height} {buffer, 2} {buffer, 3}

# Variant 1: With one tmp buffer
[kernel.v1]
kernel_file = examples/gaussian5x5/cl/gaussian1.cl
kernel_function = gaussian5x5_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
kernel_args = {buffer, 0} {size_t, 0.size} {int, src_width} {int, src_height} {buffer, 2} {buffer, 3}

# Variant 2: Two-pass with two tmp buffers
[kernel.v2]
kernel_file = examples/gaussian5x5/cl/gaussian2.cl
kernel_function = gaussian5x5_horizontal
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
kernel_args = {buffer, 0} {size_t, 0.size} {buffer, 1} {size_t, 1.size} {int, src_width} {int, src_height} {buffer, 2} {buffer, 3}
```

**Resulting kernel signatures**:

**Variant 0**:
```c
__kernel void gaussian5x5(__global const uchar* input,    // arg 0 (implicit)
                          __global uchar* output,          // arg 1 (implicit)
                          int width,                       // arg 2
                          int height,                      // arg 3
                          __global const float* kernel_x,  // arg 4 (buffer[2])
                          __global const float* kernel_y)  // arg 5 (buffer[3])
```

**Variant 1**:
```c
__kernel void gaussian5x5_optimized(__global const uchar* input,    // arg 0 (implicit)
                                    __global uchar* output,          // arg 1 (implicit)
                                    __global uchar* tmp_buffer,      // arg 2 (buffer[0])
                                    __private unsigned long tmp_size, // arg 3 (buffer[0].size)
                                    int width,                       // arg 4
                                    int height,                      // arg 5
                                    __global const float* kernel_x,  // arg 6 (buffer[2])
                                    __global const float* kernel_y)  // arg 7 (buffer[3])
```

**Variant 2**:
```c
__kernel void gaussian5x5_horizontal(__global const uchar* input,     // arg 0 (implicit)
                                     __global uchar* output,           // arg 1 (implicit)
                                     __global uchar* tmp_buffer,       // arg 2 (buffer[0])
                                     __private unsigned long tmp_size,  // arg 3 (buffer[0].size)
                                     __global uchar* tmp_buffer2,      // arg 4 (buffer[1])
                                     __private unsigned long tmp_size2, // arg 5 (buffer[1].size)
                                     int width,                        // arg 6
                                     int height,                       // arg 7
                                     __global const float* kernel_x,   // arg 8 (buffer[2])
                                     __global const float* kernel_y)   // arg 9 (buffer[3])
```

## Available OpParams Fields

The following integer fields can be referenced in `{int, field_name}`:

- `src_width` - Source image width
- `src_height` - Source image height
- `src_stride` - Source image stride (bytes per row)
- `dst_width` - Destination image width
- `dst_height` - Destination image height
- `dst_stride` - Destination image stride

## Migration Guide

### Before (Old Approach)

You had to write a custom `SetKernelArgs` function:

```c
int Gaussian5x5SetKernelArgs(cl_kernel kernel, cl_mem input_buf,
                              cl_mem output_buf, const OpParams* params) {
    CustomBuffers* custom_buffers = params->custom_buffers;
    int arg_idx = 0;

    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf);

    switch (params->kernel_variant) {
        case 0:
            clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width);
            clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height);
            clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem),
                          &custom_buffers->buffers[1].buffer);
            clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem),
                          &custom_buffers->buffers[2].buffer);
            break;
        // ... more variants
    }
    return 0;
}
```

And register it:
```c
REGISTER_ALGORITHM(
    .name = "Gaussian 5x5",
    .id = "gaussian5x5",
    .reference_impl = Gaussian5x5Ref,
    .set_kernel_args = Gaussian5x5SetKernelArgs,
    .verify_result = Gaussian5x5Verify
);
```

### After (New Approach)

Just add `kernel_args` to your `.ini` file:

```ini
[kernel.v0]
kernel_args = {int, src_width} {int, src_height} {buffer, 1} {buffer, 2}
```

Registration is simplified (auto-generated):
```c
static Algorithm gaussian5x5_algorithm = {
    .name = "Gaussian 5x5",
    .id = "gaussian5x5",
    .reference_impl = Gaussian5x5Ref,
    .verify_result = Gaussian5x5Verify
};
```

## Best Practices

1. **Use buffer indices for complex algorithms**: When different variants need buffers in different orders, use numeric indices instead of names for clarity.

2. **Document buffer indices**: Add comments in the config file showing the buffer index:
   ```ini
   [buffer.tmp_global]  # index 0
   type = READ_WRITE
   ```

3. **Match OpenCL kernel signatures exactly**: The order in `kernel_args` must match your kernel function parameters (after input/output).

4. **Use descriptive buffer names**: Even when referencing by index, use clear names in buffer sections:
   ```ini
   [buffer.horizontal_tmp]  # Better than [buffer.tmp1]
   ```

5. **Test each variant**: Ensure all variants pass verification to confirm correct argument order.

## Debugging

To see the argument setting order, the framework prints debug information:

```
=== Setting Kernel Arguments (variant: 1) ===
Total kernel_args to set: 6
kernel_args[0]: type=3, source='0'           → buffer at index 0
kernel_args[1]: type=6, source='0.size'      → size_t
kernel_args[2]: type=4, source='src_width'   → int
kernel_args[3]: type=4, source='src_height'  → int
kernel_args[4]: type=3, source='2'           → buffer at index 2
kernel_args[5]: type=3, source='3'           → buffer at index 3
```

Type codes:
- `type=3`: BUFFER_CUSTOM
- `type=4`: SCALAR_INT
- `type=6`: SCALAR_SIZE

## Implementation Details

### Code Structure

- **Config parsing**: `src/utils/config.c::ParseKernelArgs()`
- **Argument setting**: `src/platform/opencl_utils.c::OpenclSetKernelArgs()`
- **Data structures**: `src/utils/config.h::KernelArgDescriptor`

### Argument Types Enum

```c
typedef enum {
    KERNEL_ARG_TYPE_NONE = 0,
    KERNEL_ARG_TYPE_BUFFER_INPUT,
    KERNEL_ARG_TYPE_BUFFER_OUTPUT,
    KERNEL_ARG_TYPE_BUFFER_CUSTOM,
    KERNEL_ARG_TYPE_SCALAR_INT,
    KERNEL_ARG_TYPE_SCALAR_FLOAT,
    KERNEL_ARG_TYPE_SCALAR_SIZE,
} KernelArgType;
```

### Buffer Lookup Logic

The implementation supports both name-based and index-based lookups:

```c
// Check if source is numeric (index-based)
if (source_name[0] >= '0' && source_name[0] <= '9') {
    buffer_idx = atoi(source_name);
} else {
    // Look up by name
    for (int j = 0; j < custom_buffers->count; j++) {
        if (strcmp(custom_buffers->buffers[j].name, source_name) == 0) {
            buffer_idx = j;
            break;
        }
    }
}
```

## Limitations

- Maximum 32 kernel arguments per variant (`MAX_KERNEL_ARGS`)
- Only integer and size_t scalar types supported (float scalars not yet implemented)
- Buffer indices must be valid (0 to buffer_count-1)
- Size_t arguments must use `.size` suffix

## Future Enhancements

Potential improvements:
- Support for float scalar arguments
- Support for local memory size arguments
- Validation that kernel_args match actual kernel signature
- Per-variant buffer loading (only load buffers needed by active variant)
