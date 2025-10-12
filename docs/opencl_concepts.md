# OpenCL Concepts Explained

This document provides detailed explanations of the OpenCL concepts demonstrated in the threshold example.

## Table of Contents

1. [OpenCL Architecture Overview](#opencl-architecture-overview)
2. [Platform and Device Model](#platform-and-device-model)
3. [Execution Model](#execution-model)
4. [Memory Model](#memory-model)
5. [Kernel Configuration](#kernel-configuration)
6. [Building and Loading Kernels](#building-and-loading-kernels)
7. [Memory Allocation and Sharing](#memory-allocation-and-sharing)
8. [Data Transfer](#data-transfer)
9. [Result Retrieval](#result-retrieval)

## OpenCL Architecture Overview

OpenCL (Open Computing Language) is a framework for writing programs that execute across heterogeneous platforms (CPUs, GPUs, FPGAs, etc.). The architecture consists of:

- **Host**: Your main CPU program (C++)
- **Device**: The accelerator (GPU, CPU, etc.) that executes kernels
- **Kernel**: Function that runs in parallel on the device
- **Context**: Manages all resources (devices, memory, command queues)
- **Command Queue**: Orders operations sent to the device

```
┌─────────────────────────────────────────┐
│            Host Application             │
│         (Your C++ program)              │
└────────────┬────────────────────────────┘
             │
             │ OpenCL API
             │
┌────────────▼────────────────────────────┐
│          OpenCL Runtime                 │
│  (Platform, Context, Queue, Memory)     │
└────────────┬────────────────────────────┘
             │
             │ Commands
             │
┌────────────▼────────────────────────────┐
│        Device (GPU/CPU/etc)             │
│      Executes Kernels in Parallel       │
└─────────────────────────────────────────┘
```

## Platform and Device Model

### Platform

A **platform** represents an OpenCL implementation from a specific vendor (Intel, NVIDIA, AMD, etc.). A system can have multiple platforms installed.

```cpp
cl_platform_id platform;
clGetPlatformIDs(1, &platform, NULL);
```

Common platforms:
- Intel OpenCL (for Intel CPUs/GPUs)
- NVIDIA CUDA (for NVIDIA GPUs)
- AMD ROCm (for AMD GPUs)
- Portable Computing Language (POCL) - portable CPU implementation

### Device

A **device** is a specific compute unit within a platform (e.g., a specific GPU or CPU).

```cpp
cl_device_id device;
clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
```

Device types:
- `CL_DEVICE_TYPE_GPU` - Graphics processors
- `CL_DEVICE_TYPE_CPU` - CPUs
- `CL_DEVICE_TYPE_ACCELERATOR` - Dedicated accelerators
- `CL_DEVICE_TYPE_ALL` - All available devices

### Context

A **context** is a container that manages:
- One or more devices
- Memory objects (buffers, images)
- Command queues
- Programs and kernels

```cpp
cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
```

Think of context as the "workspace" for all your OpenCL operations.

### Command Queue

A **command queue** is used to submit operations to a device:
- Memory transfers
- Kernel executions
- Synchronization commands

```cpp
cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
```

Commands can execute:
- **In-order**: Commands execute in submission order (default)
- **Out-of-order**: Runtime can reorder for optimization

## Execution Model

### Work Items, Work Groups, and NDRange

OpenCL executes kernels using a parallel execution model:

```
Global Work Size (NDRange)
┌─────────────────────────────────────┐
│  Work Group 0    Work Group 1       │
│ ┌─────────────┐ ┌─────────────┐    │
│ │ WI WI WI WI │ │ WI WI WI WI │    │
│ │ WI WI WI WI │ │ WI WI WI WI │    │
│ │ WI WI WI WI │ │ WI WI WI WI │    │
│ └─────────────┘ └─────────────┘    │
└─────────────────────────────────────┘

WI = Work Item (individual thread)
```

**Work Item**: A single instance of the kernel (like one thread)
- Has a unique global ID: `get_global_id(dimension)`
- Has a local ID within its work group: `get_local_id(dimension)`

**Work Group**: A collection of work items that can cooperate
- Share local memory
- Can synchronize using barriers

**NDRange**: The total index space (global work size)
- Can be 1D, 2D, or 3D
- In our example: 2D array representing image pixels

### Example: 2D Image Processing

For a 512x512 image:

```cpp
// Define global work size (total work items)
size_t globalWorkSize[2] = { 512, 512 };  // 262,144 work items

// Execute kernel
clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, ...);
```

Each work item processes one pixel:

```c
// In kernel
int x = get_global_id(0);  // 0 to 511
int y = get_global_id(1);  // 0 to 511
int idx = y * width + x;   // Linear index
```

## Memory Model

OpenCL has a hierarchical memory model:

```
┌──────────────────────────────────────────┐
│         Host Memory (RAM)                │
│         - Accessible by CPU              │
└──────────────┬───────────────────────────┘
               │ PCIe Bus
               │ (Slow Transfer)
┌──────────────▼───────────────────────────┐
│         Global Memory (VRAM)             │
│         - Accessible by all work items   │
│         - Large but slower               │
│         - Persists across kernel calls   │
└──────────────┬───────────────────────────┘
               │
┌──────────────▼───────────────────────────┐
│         Constant Memory                  │
│         - Read-only, cached              │
│         - Fast for read-only data        │
└──────────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────┐
│         Local Memory (Shared)            │
│         - Shared within work group       │
│         - Very fast                      │
│         - Small size (typically KB)      │
└──────────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────┐
│         Private Memory (Registers)       │
│         - Per work item                  │
│         - Fastest                        │
│         - Variables in kernel            │
└──────────────────────────────────────────┘
```

### Memory Types in Kernels

```c
__kernel void example(
    __global int* data,      // Global: device main memory
    __constant float* params, // Constant: read-only cached
    __local int* scratch)     // Local: shared within work group
{
    int private_var = 0;      // Private: in registers
}
```

### Memory Access Patterns

**Coalesced Access** (efficient):
```c
// Adjacent work items access adjacent memory
int idx = get_global_id(0);
data[idx] = value;  // Good: sequential access
```

**Strided Access** (less efficient):
```c
// Work items access with large strides
int idx = get_global_id(0) * stride;
data[idx] = value;  // Less efficient
```

## Kernel Configuration

### Writing Kernels

Kernels are written in OpenCL C (similar to C99):

```c
__kernel void my_kernel(
    __global const float* input,   // Read-only input
    __global float* output,        // Write output
    const int size)                // Scalar parameter
{
    // Get work item ID
    int gid = get_global_id(0);

    // Bounds checking
    if (gid >= size) return;

    // Do computation
    output[gid] = input[gid] * 2.0f;
}
```

### Kernel Qualifiers

- `__kernel`: Marks function as kernel entry point
- `__global`: Pointer to global memory
- `__local`: Pointer to local (shared) memory
- `__constant`: Pointer to constant memory
- `__private`: Private to work item (default for locals)

### Built-in Functions

OpenCL provides many built-in functions:

**Work Item Functions**:
```c
get_global_id(dim)    // Global index
get_local_id(dim)     // Local index in work group
get_group_id(dim)     // Work group index
get_global_size(dim)  // Total work items
get_local_size(dim)   // Work group size
```

**Math Functions**:
```c
sin, cos, sqrt, pow, fabs, floor, ceil, etc.
```

**Vector Operations**:
```c
float4 vec = (float4)(1.0f, 2.0f, 3.0f, 4.0f);
vec.xy;  // Access components
```

## Building and Loading Kernels

There are two ways to use kernels:

### 1. Runtime Compilation (Our Example)

Load kernel source from file and compile at runtime:

```cpp
// Load source code from file
std::string source = loadKernelSource("kernel.cl");

// Create program from source
const char* src = source.c_str();
cl_program program = clCreateProgramWithSource(context, 1, &src, NULL, &err);

// Compile the kernel
err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

// Get kernel object
cl_kernel kernel = clCreateKernel(program, "kernel_name", &err);
```

**Advantages**:
- Easy development and debugging
- Can modify kernels without recompiling host code
- Can pass compiler options at runtime

**Disadvantages**:
- Compilation time at startup
- Requires shipping source code

### 2. Binary Compilation (Production)

Pre-compile kernels and load binary:

```cpp
// Get binary after compilation
size_t binarySize;
clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binarySize, NULL);

unsigned char* binary = new unsigned char[binarySize];
clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary, NULL);

// Save binary to file...

// Later, load from binary
cl_program program = clCreateProgramWithBinary(context, 1, &device,
    &binarySize, (const unsigned char**)&binary, NULL, &err);
```

**Advantages**:
- Faster startup (no compilation)
- Protect source code

**Disadvantages**:
- Platform/device specific
- Harder to debug

### Compiler Options

You can pass options to the kernel compiler:

```cpp
const char* options = "-cl-fast-relaxed-math -cl-mad-enable";
clBuildProgram(program, 1, &device, options, NULL, NULL);
```

Common options:
- `-cl-fast-relaxed-math`: Less precise but faster math
- `-cl-mad-enable`: Enable multiply-add optimization
- `-cl-finite-math-only`: Assume no NaN/Infinity
- `-D MACRO=value`: Define preprocessor macros

## Memory Allocation and Sharing

### Creating Buffers

Buffers are allocated on the device:

```cpp
cl_mem buffer = clCreateBuffer(
    context,           // Context
    CL_MEM_READ_ONLY,  // Flags
    size_in_bytes,     // Size
    host_ptr,          // Optional host data
    &err               // Error code
);
```

### Memory Flags

Control how memory is allocated and accessed:

**Access Flags**:
- `CL_MEM_READ_ONLY`: Kernel only reads
- `CL_MEM_WRITE_ONLY`: Kernel only writes
- `CL_MEM_READ_WRITE`: Kernel reads and writes

**Host Access Flags**:
- `CL_MEM_HOST_READ_ONLY`: Host only reads
- `CL_MEM_HOST_WRITE_ONLY`: Host only writes
- `CL_MEM_HOST_NO_ACCESS`: Host doesn't access

**Allocation Flags**:
- `CL_MEM_USE_HOST_PTR`: Use provided host memory
- `CL_MEM_ALLOC_HOST_PTR`: Allocate host-accessible memory
- `CL_MEM_COPY_HOST_PTR`: Copy from host memory

### Example: Different Memory Patterns

**Pattern 1: Separate host and device memory** (our example):
```cpp
std::vector<int> hostData(1000);

// Allocate device memory (separate from host)
cl_mem deviceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
    1000 * sizeof(int), NULL, &err);

// Copy host to device
clEnqueueWriteBuffer(queue, deviceBuffer, CL_TRUE, 0,
    1000 * sizeof(int), hostData.data(), 0, NULL, NULL);
```

**Pattern 2: Shared/mapped memory**:
```cpp
// Allocate memory that can be mapped to host
cl_mem buffer = clCreateBuffer(context,
    CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
    1000 * sizeof(int), NULL, &err);

// Map to host address space
int* ptr = (int*)clEnqueueMapBuffer(queue, buffer, CL_TRUE,
    CL_MAP_READ | CL_MAP_WRITE, 0, 1000 * sizeof(int), 0, NULL, NULL, &err);

// Use ptr like normal memory
ptr[0] = 42;

// Unmap when done
clEnqueueUnmapMemObject(queue, buffer, ptr, 0, NULL, NULL);
```

**Pattern 3: Zero-copy (for integrated GPUs)**:
```cpp
// Use host memory directly (efficient on integrated GPUs)
std::vector<int> hostData(1000);

cl_mem buffer = clCreateBuffer(context,
    CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
    1000 * sizeof(int), hostData.data(), &err);
```

## Data Transfer

### Explicit Transfers

**Host to Device**:
```cpp
clEnqueueWriteBuffer(
    queue,              // Command queue
    buffer,             // Device buffer
    CL_TRUE,            // Blocking (wait for completion)
    0,                  // Offset in buffer
    size_in_bytes,      // Number of bytes
    host_ptr,           // Source host pointer
    0, NULL, NULL       // Events (for dependencies)
);
```

**Device to Host**:
```cpp
clEnqueueReadBuffer(
    queue,              // Command queue
    buffer,             // Device buffer
    CL_TRUE,            // Blocking
    0,                  // Offset
    size_in_bytes,      // Bytes to read
    host_ptr,           // Destination host pointer
    0, NULL, NULL       // Events
);
```

### Blocking vs Non-blocking

**Blocking** (`CL_TRUE`):
- Function waits until transfer completes
- Simpler to use
- May be less efficient

**Non-blocking** (`CL_FALSE`):
- Function returns immediately
- Must synchronize manually (events or clFinish)
- Better performance with pipelining

Example:
```cpp
// Non-blocking transfer
cl_event event;
clEnqueueWriteBuffer(queue, buffer, CL_FALSE, 0, size,
    data, 0, NULL, &event);

// Do other work...

// Wait for transfer to complete
clWaitForEvents(1, &event);
clReleaseEvent(event);
```

### Transfer Performance Tips

1. **Minimize transfers**: Keep data on device when possible
2. **Use async transfers**: Overlap computation and communication
3. **Batch transfers**: One large transfer is faster than many small ones
4. **Pinned memory**: Use `CL_MEM_ALLOC_HOST_PTR` for faster transfers

## Result Retrieval

### Reading Results

After kernel execution, retrieve results from device:

```cpp
// 1. Execute kernel
clEnqueueNDRangeKernel(queue, kernel, ...);

// 2. Wait for completion
clFinish(queue);

// 3. Read results back to host
std::vector<float> results(size);
clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0,
    size * sizeof(float), results.data(), 0, NULL, NULL);
```

### Synchronization Methods

**Method 1: Blocking operations**:
```cpp
// CL_TRUE makes it wait
clEnqueueReadBuffer(queue, buffer, CL_TRUE, ...);
// Buffer is ready here
```

**Method 2: clFinish**:
```cpp
clEnqueueReadBuffer(queue, buffer, CL_FALSE, ...);
clFinish(queue);  // Wait for all commands in queue
// Buffer is ready here
```

**Method 3: Events**:
```cpp
cl_event readEvent;
clEnqueueReadBuffer(queue, buffer, CL_FALSE, 0, size,
    data, 0, NULL, &readEvent);

// Do other work...

clWaitForEvents(1, &readEvent);
// Buffer is ready here
clReleaseEvent(readEvent);
```

### Event Dependencies

Create execution pipelines with events:

```cpp
cl_event writeEvent, kernelEvent, readEvent;

// 1. Transfer data to device
clEnqueueWriteBuffer(queue, inputBuffer, CL_FALSE, 0, size,
    input, 0, NULL, &writeEvent);

// 2. Execute kernel (depends on write completing)
clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, NULL,
    1, &writeEvent, &kernelEvent);

// 3. Read results (depends on kernel completing)
clEnqueueReadBuffer(queue, outputBuffer, CL_FALSE, 0, size,
    output, 1, &kernelEvent, &readEvent);

// Wait for everything
clWaitForEvents(1, &readEvent);

// Cleanup
clReleaseEvent(writeEvent);
clReleaseEvent(kernelEvent);
clReleaseEvent(readEvent);
```

### Profiling

Enable profiling to measure execution time:

```cpp
// Create queue with profiling enabled
cl_command_queue queue = clCreateCommandQueue(context, device,
    CL_QUEUE_PROFILING_ENABLE, &err);

cl_event event;
clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, NULL,
    0, NULL, &event);

clWaitForEvents(1, &event);

// Get timing information
cl_ulong startTime, endTime;
clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
    sizeof(cl_ulong), &startTime, NULL);
clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
    sizeof(cl_ulong), &endTime, NULL);

double executionTime = (endTime - startTime) * 1.0e-9;  // Convert to seconds
std::cout << "Kernel execution time: " << executionTime << " seconds" << std::endl;

clReleaseEvent(event);
```

## Summary: Complete Workflow

Here's the complete OpenCL workflow recap:

```cpp
// 1. Setup
cl_platform_id platform;
cl_device_id device;
clGetPlatformIDs(1, &platform, NULL);
clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

// 2. Create context and queue
cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);

// 3. Build kernel
cl_program program = clCreateProgramWithSource(context, 1, &source, NULL, &err);
clBuildProgram(program, 1, &device, NULL, NULL, NULL);
cl_kernel kernel = clCreateKernel(program, "kernel_name", &err);

// 4. Allocate memory
cl_mem inputBuf = clCreateBuffer(context, CL_MEM_READ_ONLY, size, NULL, &err);
cl_mem outputBuf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, NULL, &err);

// 5. Transfer data to device
clEnqueueWriteBuffer(queue, inputBuf, CL_TRUE, 0, size, hostData, 0, NULL, NULL);

// 6. Set kernel arguments
clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuf);
clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuf);

// 7. Execute kernel
size_t globalSize = 1024;
clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, NULL, 0, NULL, NULL);

// 8. Get results
clEnqueueReadBuffer(queue, outputBuf, CL_TRUE, 0, size, results, 0, NULL, NULL);

// 9. Cleanup
clReleaseMemObject(inputBuf);
clReleaseMemObject(outputBuf);
clReleaseKernel(kernel);
clReleaseProgram(program);
clReleaseCommandQueue(queue);
clReleaseContext(context);
```

This workflow maps directly to our threshold example in `src/main.cpp`!
