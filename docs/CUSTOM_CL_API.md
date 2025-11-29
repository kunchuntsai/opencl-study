# Custom OpenCL Host API Framework

This framework provides a flexible system for using custom OpenCL host API implementations alongside standard OpenCL APIs, configurable via `.ini` files.

## Overview

The custom CL API framework allows you to:
- Switch between standard OpenCL API and custom extension API on a per-kernel basis
- Implement custom scheduling, profiling, or optimization logic
- Extend OpenCL functionality with hardware-specific optimizations

## Configuration

### Setting Host Type in .ini Files

Each kernel variant can specify which host API to use via the `host_type` parameter:

```ini
[kernel.v0]
host_type = standard      # Use standard OpenCL API (default)
kernel_file = src/algorithm/kernel.cl
kernel_function = my_kernel
work_dim = 2
global_work_size = 1920,1080
local_work_size = 16,16

[kernel.v1]
host_type = cl_extension  # Use custom CL extension API
kernel_file = src/algorithm/kernel_optimized.cl
kernel_function = my_kernel_optimized
work_dim = 2
global_work_size = 1920,1080
local_work_size = 16,16
```

### Valid Values

- `standard` (default) - Uses standard OpenCL APIs
- `cl_extension` - Uses custom CL extension API framework

If `host_type` is not specified, it defaults to `standard`.

## Architecture

### File Structure

```
src/utils/
├── cl_extension_api.h       # Custom CL extension API interface
├── cl_extension_api.c       # Dummy implementation (replace with your logic)
├── config.h                 # HostType enum and configuration structures
├── config.c                 # Configuration parser (handles host_type)
├── opencl_utils.h           # OpenCL utility functions
└── opencl_utils.c           # Kernel execution (routes to standard or custom API)
```

### Key Components

#### 1. HostType Enum (config.h)
```c
typedef enum {
    HOST_TYPE_STANDARD = 0,     // Standard OpenCL API (default)
    HOST_TYPE_CL_EXTENSION      // Custom CL extension API
} HostType;
```

#### 2. KernelConfig Structure (config.h)
Each kernel configuration includes a `host_type` field:
```c
typedef struct {
    char variant_id[32];
    char kernel_file[256];
    char kernel_function[64];
    int work_dim;
    size_t global_work_size[3];
    size_t local_work_size[3];
    HostType host_type;         // <-- Host API selection
} KernelConfig;
```

#### 3. Custom CL Extension Context (cl_extension_api.h)
```c
typedef struct {
    void* extension_data;       // Custom extension-specific data
    int initialized;            // Initialization flag
} CLExtensionContext;
```

#### 4. Custom API Functions (cl_extension_api.h)

The framework provides custom versions of key OpenCL functions:

- `cl_extension_init()` - Initialize custom context
- `cl_extension_cleanup()` - Cleanup custom context
- `cl_extension_enqueue_ndrange_kernel()` - Custom kernel execution
- `cl_extension_create_buffer()` - Custom buffer creation
- `cl_extension_finish()` - Custom synchronization
- `cl_extension_print_info()` - Display extension information

### Execution Flow

1. **Initialization** (`opencl_init()`)
   - Standard OpenCL context/queue created
   - Custom CL extension context initialized

2. **Kernel Execution** (`opencl_run_kernel()`)
   - Configuration specifies `host_type`
   - If `HOST_TYPE_STANDARD`: Use `clEnqueueNDRangeKernel()`, `clFinish()`
   - If `HOST_TYPE_CL_EXTENSION`: Use `cl_extension_enqueue_ndrange_kernel()`, `cl_extension_finish()`

3. **Cleanup** (`opencl_cleanup()`)
   - Custom CL extension context cleaned up
   - Standard OpenCL resources released

## Implementing Custom Logic

### Current Implementation

The current implementation in `cl_extension_api.c` contains **dummy code** that:
- Logs information about API calls
- Delegates to standard OpenCL APIs

Example from `cl_extension_enqueue_ndrange_kernel()`:
```c
/* Dummy implementation: Print information and call standard API */
printf("[CL Extension] Custom enqueue_ndrange_kernel called\n");
printf("[CL Extension]   Work dimensions: %u\n", work_dim);

/* TODO: Add custom scheduling/optimization logic here */
/* For now, just call the standard OpenCL API */
err = clEnqueueNDRangeKernel(
    command_queue,
    kernel,
    work_dim,
    global_work_offset,
    global_work_size,
    local_work_size,
    num_events_in_wait_list,
    event_wait_list,
    event);
```

### Customization Steps

To implement your own custom CL API logic:

1. **Edit `cl_extension_api.c`**
   - Replace dummy implementations with your custom logic
   - Add hardware-specific optimizations
   - Implement custom scheduling/profiling

2. **Extend `CLExtensionContext`** (if needed)
   ```c
   typedef struct {
       void* custom_allocator;
       CustomScheduler* scheduler;
       ProfilingData* profiling;
       int initialized;
   } CLExtensionContext;
   ```

3. **Add New Custom Functions** (if needed)
   - Define new APIs in `cl_extension_api.h`
   - Implement in `cl_extension_api.c`
   - Call from `opencl_utils.c` as needed

## Example Usage

### Test with Standard API
```bash
./build/opencl_host gaussian5x5 0   # Uses variant v0 (standard)
```

Output:
```
=== Using Standard OpenCL API ===
GPU kernel time: 0.009 ms
```

### Test with Custom Extension API
```bash
./build/opencl_host gaussian5x5 1   # Uses variant v1 (cl_extension)
```

Output:
```
=== Using Custom CL Extension API ===
[CL Extension] Custom enqueue_ndrange_kernel called
[CL Extension]   Work dimensions: 2
[CL Extension]   Global work size: 1920 x 1088
[CL Extension]   Local work size: 16 x 16
[CL Extension] Kernel enqueued successfully
[CL Extension] Custom finish called
[CL Extension] Queue finished successfully
GPU kernel time: 0.016 ms
```

## Benefits

1. **Flexibility**: Switch between standard and custom APIs without code changes
2. **Per-Kernel Configuration**: Different kernels can use different APIs
3. **Extensibility**: Easy to add new custom API functions
4. **Debugging**: Custom API can add logging, validation, profiling
5. **Hardware Optimization**: Implement vendor-specific optimizations

## Future Extensions

Potential enhancements:
- Multiple custom API implementations (e.g., `cl_extension_vendor_a`, `cl_extension_vendor_b`)
- Runtime API selection based on hardware capabilities
- Custom memory allocators and buffer pools
- Advanced scheduling policies
- Performance profiling and analytics
