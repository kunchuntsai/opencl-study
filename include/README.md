# Public API Documentation

This directory contains the public API headers for the OpenCL Image Processing Framework. When writing new algorithm implementations in `examples/`, you should only include headers from this directory.

## Directory Structure

```
include/
├── op_interface.h        # Core algorithm interface (REQUIRED)
├── op_registry.h         # Algorithm registration macros (REQUIRED)
├── algorithm_runner.h    # Internal use only (for main.c, not for examples/)
└── utils/                # Optional utility headers
    ├── safe_ops.h        # Safe arithmetic operations
    └── verify.h          # Verification helper functions
```

## For Algorithm Developers (examples/)

When implementing a new algorithm in `examples/youralgname/c_ref/`, include **only** these headers:

### Required Headers

```c
#include "op_interface.h"    // For OpParams, Algorithm, Buffer types
#include "op_registry.h"      // For REGISTER_ALGORITHM macro
```

### Optional Utility Headers

```c
#include "utils/safe_ops.h"   // For SafeMulInt, SafeAddSize, etc.
#include "utils/verify.h"     // For VerifyExactMatch, VerifyWithTolerance
```

### Example Algorithm Implementation

```c
#include <stddef.h>

#include "op_interface.h"
#include "op_registry.h"
#include "utils/safe_ops.h"
#include "utils/verify.h"

/* Reference implementation */
void MyAlgorithmRef(const OpParams* params) {
    /* Your CPU implementation here */
}

/* Verification function */
int MyAlgorithmVerify(const OpParams* params, float* max_error) {
    return VerifyExactMatch(params->gpu_output, params->ref_output,
                           params->src_width, params->src_height, 0);
}

/* Kernel argument setter */
int MyAlgorithmSetKernelArgs(cl_kernel kernel, cl_mem input_buf,
                             cl_mem output_buf, const OpParams* params) {
    cl_int err = 0;
    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
    err |= clSetKernelArg(kernel, 2, sizeof(int), &params->src_width);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &params->src_height);
    return (err == CL_SUCCESS) ? 0 : -1;
}

/* Register the algorithm (uses op_registry.h macro) */
REGISTER_ALGORITHM(myalgorithm, "My Algorithm", MyAlgorithmRef,
                   MyAlgorithmVerify, MyAlgorithmSetKernelArgs)
```

## What NOT to Include

❌ **Do NOT include these internal headers:**
- `platform/opencl_utils.h` - Internal OpenCL utilities
- `platform/cache_manager.h` - Internal cache management
- `utils/config.h` - Internal configuration parsing
- `utils/image_io.h` - Internal image I/O
- Any headers from `src/` directory

These are internal implementation details of the framework library and should not be used by algorithm implementations.

## Header Descriptions

### op_interface.h
Defines the core types and interfaces for implementing algorithms:
- `Algorithm` - Algorithm structure with function pointers
- `OpParams` - Generic parameter structure for algorithms
- `RuntimeBuffer`, `CustomBuffers` - Buffer management types
- `BufferType`, `HostType`, `BorderMode` - Configuration enums
- `SetKernelArgsFunc` - Function pointer type for kernel argument setters

### op_registry.h
Provides the registration system:
- `REGISTER_ALGORITHM` - Macro to automatically register your algorithm
- `FindAlgorithm`, `GetAlgorithmCount`, `ListAlgorithms` - Registry query functions

### utils/safe_ops.h (Optional)
Safe arithmetic operations with overflow checking:
- `SafeMulInt` - Safe integer multiplication
- `SafeMulSize` - Safe size_t multiplication
- `SafeAddSize` - Safe size_t addition
- `SafeStrtol` - Safe string to long conversion

### utils/verify.h (Optional)
Verification helpers for comparing GPU vs CPU results:
- `VerifyExactMatch` - Exact pixel-by-pixel comparison
- `VerifyWithTolerance` - Comparison with tolerance for floating-point ops

### algorithm_runner.h (Internal Only)
**For framework internal use (main.c) only.** Algorithm implementations should NOT include this header. It contains forward declarations for internal types and is used by the main application, not by algorithms.

## Building Your Algorithm

Your algorithm in `examples/youralgname/c_ref/youralgname_ref.c` will be automatically:
1. Discovered by CMake during build
2. Compiled and linked against the framework library
3. Registered via the `REGISTER_ALGORITHM` macro
4. Available for execution via config files

No manual CMakeLists.txt modification needed!

## Library Architecture

The framework is built as a static library (`libopencl_imgproc.a`) containing:
- **core/** - Algorithm execution pipeline and registry
- **platform/** - OpenCL platform abstraction
- **utils/** - Utility functions

Your algorithm code links against this library and only needs access to the public API in `include/`.
