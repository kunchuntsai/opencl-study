# Custom OpenCL API Integration Guide

## Overview

This guide provides best practices for integrating custom OpenCL API wrappers (e.g., custom `clEnqueueNDRangeKernel()`) alongside standard OpenCL APIs. Different kernel variants may require different execution strategies based on their implementation characteristics.

## Use Case

Consider a scenario where:
- **dilate0.cl** - Standard kernel using basic OpenCL APIs
- **dilate1.cl** - Optimized kernel requiring custom API wrapper for specialized features (e.g., enhanced profiling, custom memory management, vendor-specific optimizations)

The challenge is to maintain a clean architecture that supports both execution paths without duplicating code or creating tight coupling.

---

## Approach 1: Config-Driven API Selection ⭐ **RECOMMENDED**

### Description

Extend the configuration system to explicitly declare which execution mode each kernel variant requires. The config file drives the API selection logic.

### Implementation

**Configuration (`config/dilate3x3.ini`):**
```ini
[kernel.v0]
kernel_file = src/dilate/cl/dilate0.cl
kernel_function = dilate3x3
execution_mode = standard       # Uses clEnqueueNDRangeKernel
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

[kernel.v1]
kernel_file = src/dilate/cl/dilate1.cl
kernel_function = dilate3x3_optimized
execution_mode = custom         # Uses custom wrapper
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

**Code (`src/utils/config_parser.h`):**
```c
typedef enum {
    EXEC_MODE_STANDARD,
    EXEC_MODE_CUSTOM,
    EXEC_MODE_ASYNC      // Future extension example
} ExecutionMode;

typedef struct {
    char variant_id[64];
    char kernel_file[256];
    char kernel_function[64];
    int work_dim;
    size_t global_work_size[3];
    size_t local_work_size[3];
    ExecutionMode exec_mode;  // NEW
} KernelConfig;
```

**Execution (`src/utils/opencl_utils.c`):**
```c
int opencl_run_kernel(OpenCLEnv* env, cl_kernel kernel,
                      cl_mem input_buf, cl_mem output_buf,
                      int width, int height,
                      const size_t* global_work_size,
                      const size_t* local_work_size,
                      int work_dim,
                      ExecutionMode exec_mode,
                      double* gpu_time_ms) {

    cl_int err;
    cl_event event;

    /* Set kernel arguments */
    // ... existing argument setup code ...

    /* Execute kernel with appropriate API */
    if (exec_mode == EXEC_MODE_CUSTOM) {
        err = custom_clEnqueueNDRangeKernel(env->queue, kernel,
                                           (cl_uint)work_dim,
                                           NULL, global_work_size,
                                           local_work_size,
                                           0U, NULL, &event);
    } else {
        err = clEnqueueNDRangeKernel(env->queue, kernel,
                                    (cl_uint)work_dim,
                                    NULL, global_work_size,
                                    local_work_size,
                                    0U, NULL, &event);
    }

    /* Rest of execution and timing code */
    // ...
}
```

### Pros
- ✅ **Declarative and explicit** - Execution mode is clearly documented in config
- ✅ **Aligns with existing architecture** - Extends the config-driven design pattern
- ✅ **Easy to extend** - Add new execution modes by extending the enum
- ✅ **No runtime detection overhead** - Decision made at config parse time
- ✅ **Simple testing** - Easy to test different modes by changing config
- ✅ **Self-documenting** - Config file serves as documentation
- ✅ **Centralized control** - All kernel behavior controlled from config

### Cons
- ❌ **Manual configuration required** - Must explicitly set execution mode for each variant
- ❌ **Config file grows** - Adds another field to each kernel variant
- ❌ **Potential misconfiguration** - User could select wrong execution mode for a kernel
- ❌ **Tight coupling to config** - Changes require config file updates

### When to Use
- You have a config-driven architecture (like this project)
- Execution mode is a known characteristic of each kernel variant
- You want explicit control over which API each kernel uses
- You need to support multiple execution strategies long-term

---

## Approach 2: Function Pointer Registry

### Description

Register different execution strategies as function pointers, allowing runtime selection of the appropriate executor based on kernel configuration.

### Implementation

**Registry (`src/utils/opencl_utils.h`):**
```c
typedef int (*KernelExecutor)(OpenCLEnv* env, cl_kernel kernel,
                             cl_mem input_buf, cl_mem output_buf,
                             int width, int height,
                             const size_t* global_work_size,
                             const size_t* local_work_size,
                             int work_dim,
                             double* gpu_time_ms);
```

**Executors (`src/utils/opencl_utils.c`):**
```c
static int execute_standard(OpenCLEnv* env, cl_kernel kernel,
                           cl_mem input_buf, cl_mem output_buf,
                           int width, int height,
                           const size_t* global_work_size,
                           const size_t* local_work_size,
                           int work_dim,
                           double* gpu_time_ms) {
    // Standard clEnqueueNDRangeKernel implementation
    cl_int err;
    cl_event event;

    /* Set kernel arguments */
    // ...

    err = clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim,
                                NULL, global_work_size, local_work_size,
                                0U, NULL, &event);
    // ... profiling and timing ...

    return 0;
}

static int execute_custom(OpenCLEnv* env, cl_kernel kernel,
                         cl_mem input_buf, cl_mem output_buf,
                         int width, int height,
                         const size_t* global_work_size,
                         const size_t* local_work_size,
                         int work_dim,
                         double* gpu_time_ms) {
    // Custom API implementation
    cl_int err;
    cl_event event;

    /* Set kernel arguments */
    // ...

    err = custom_clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim,
                                       NULL, global_work_size, local_work_size,
                                       0U, NULL, &event);
    // ... profiling and timing ...

    return 0;
}

static KernelExecutor get_executor(const char* execution_mode) {
    if (strcmp(execution_mode, "custom") == 0) {
        return execute_custom;
    } else if (strcmp(execution_mode, "async") == 0) {
        return execute_async;
    }
    return execute_standard;
}
```

**Usage (`src/main.c`):**
```c
static void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                         const Config* config, OpenCLEnv* env) {
    // ... setup code ...

    /* Get appropriate executor for this kernel */
    KernelExecutor executor = get_executor(kernel_cfg->execution_mode);

    /* Run kernel using selected executor */
    run_result = executor(env, kernel, input_buf, output_buf,
                         config->src_width, config->src_height,
                         kernel_cfg->global_work_size,
                         kernel_cfg->local_work_size,
                         kernel_cfg->work_dim,
                         &gpu_time);

    // ... rest of function ...
}
```

### Pros
- ✅ **Maximum flexibility** - Easy to add new execution strategies
- ✅ **Clean separation of concerns** - Each executor is isolated
- ✅ **Strategy pattern** - Follows well-established design pattern
- ✅ **Testable** - Each executor can be unit tested independently
- ✅ **Runtime selection** - Can choose executor based on runtime conditions
- ✅ **No code duplication** - Common setup code can be shared

### Cons
- ❌ **More complex** - Requires understanding of function pointers
- ❌ **Harder to debug** - Indirect calls through function pointers
- ❌ **Memory overhead** - Function pointer storage (minimal)
- ❌ **Still requires config** - Execution mode must be specified somewhere
- ❌ **Potential performance cost** - Indirect function call overhead (negligible)
- ❌ **More files/functions** - Increases codebase size

### When to Use
- You need extreme flexibility in execution strategies
- You want to easily add new execution modes without modifying core logic
- You're comfortable with function pointers and strategy pattern
- You need to support runtime selection of executors

---

## Approach 3: Kernel Attribute Detection (Runtime)

### Description

Automatically detect which API to use based on kernel characteristics queried at runtime (e.g., local memory usage, work group size requirements).

### Implementation

**Detection Logic (`src/utils/opencl_utils.c`):**
```c
static int requires_custom_api(OpenCLEnv* env, cl_kernel kernel) {
    cl_int err;
    size_t local_mem_size;
    size_t preferred_work_group_multiple;

    /* Query kernel local memory usage */
    err = clGetKernelWorkGroupInfo(kernel, env->device,
                                   CL_KERNEL_LOCAL_MEM_SIZE,
                                   sizeof(local_mem_size),
                                   &local_mem_size, NULL);
    if (err != CL_SUCCESS) {
        return 0;  /* Default to standard API */
    }

    /* Custom API needed for kernels with >8KB local memory */
    if (local_mem_size > 8192) {
        return 1;
    }

    /* Query preferred work group size multiple */
    err = clGetKernelWorkGroupInfo(kernel, env->device,
                                   CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                   sizeof(preferred_work_group_multiple),
                                   &preferred_work_group_multiple, NULL);
    if (err != CL_SUCCESS) {
        return 0;
    }

    /* Custom API for non-standard work group multiples */
    if (preferred_work_group_multiple > 64) {
        return 1;
    }

    return 0;
}

int opencl_run_kernel(OpenCLEnv* env, cl_kernel kernel,
                      cl_mem input_buf, cl_mem output_buf,
                      int width, int height,
                      const size_t* global_work_size,
                      const size_t* local_work_size,
                      int work_dim,
                      double* gpu_time_ms) {

    cl_int err;
    cl_event event;

    /* Set kernel arguments */
    // ... existing setup code ...

    /* Auto-detect which API to use */
    if (requires_custom_api(env, kernel)) {
        printf("Using custom API for this kernel\n");
        err = custom_clEnqueueNDRangeKernel(env->queue, kernel,
                                           (cl_uint)work_dim,
                                           NULL, global_work_size,
                                           local_work_size,
                                           0U, NULL, &event);
    } else {
        err = clEnqueueNDRangeKernel(env->queue, kernel,
                                    (cl_uint)work_dim,
                                    NULL, global_work_size,
                                    local_work_size,
                                    0U, NULL, &event);
    }

    /* Rest of function */
    // ...
}
```

### Pros
- ✅ **Automatic detection** - No manual configuration needed
- ✅ **Adaptive** - Responds to actual kernel characteristics
- ✅ **No config changes** - Works with existing config structure
- ✅ **Self-optimizing** - Always selects appropriate API
- ✅ **Reduced user error** - No chance of selecting wrong mode

### Cons
- ❌ **Less explicit** - Not obvious which API will be used
- ❌ **Harder to debug** - Behavior changes based on kernel characteristics
- ❌ **Runtime overhead** - Query operations on every kernel execution
- ❌ **Complex detection logic** - Heuristics may not cover all cases
- ❌ **Inflexible** - Can't override automatic decision
- ❌ **Unpredictable** - Same config might behave differently on different hardware
- ❌ **Testing challenges** - Harder to test specific execution paths

### When to Use
- You want fully automatic API selection
- Kernel characteristics reliably determine which API to use
- You're willing to accept the runtime overhead
- Users shouldn't need to understand execution modes

---

## Approach 4: Separate Execution Functions

### Description

Create distinct, explicitly-named functions for each API variant. Caller chooses which function to call.

### Implementation

**API (`src/utils/opencl_utils.h`):**
```c
/* Standard OpenCL API execution */
int opencl_run_kernel_standard(OpenCLEnv* env, cl_kernel kernel,
                               cl_mem input_buf, cl_mem output_buf,
                               int width, int height,
                               const size_t* global_work_size,
                               const size_t* local_work_size,
                               int work_dim,
                               double* gpu_time_ms);

/* Custom OpenCL API execution */
int opencl_run_kernel_custom(OpenCLEnv* env, cl_kernel kernel,
                            cl_mem input_buf, cl_mem output_buf,
                            int width, int height,
                            const size_t* global_work_size,
                            const size_t* local_work_size,
                            int work_dim,
                            double* gpu_time_ms);
```

**Implementation (`src/utils/opencl_utils.c`):**
```c
int opencl_run_kernel_standard(OpenCLEnv* env, cl_kernel kernel, /*...*/) {
    cl_int err;
    cl_event event;

    /* Set kernel arguments */
    // ... common setup ...

    /* Execute with standard API */
    err = clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim,
                                NULL, global_work_size, local_work_size,
                                0U, NULL, &event);

    /* Profiling and timing */
    // ... common code ...

    return 0;
}

int opencl_run_kernel_custom(OpenCLEnv* env, cl_kernel kernel, /*...*/) {
    cl_int err;
    cl_event event;

    /* Set kernel arguments */
    // ... common setup (DUPLICATED) ...

    /* Execute with custom API */
    err = custom_clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim,
                                       NULL, global_work_size, local_work_size,
                                       0U, NULL, &event);

    /* Profiling and timing */
    // ... common code (DUPLICATED) ...

    return 0;
}
```

**Usage (`src/main.c`):**
```c
static void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                         const Config* config, OpenCLEnv* env) {
    // ... setup code ...

    /* Select execution function based on config or variant */
    if (strcmp(kernel_cfg->execution_mode, "custom") == 0) {
        run_result = opencl_run_kernel_custom(env, kernel, input_buf, output_buf,
                                             config->src_width, config->src_height,
                                             kernel_cfg->global_work_size,
                                             kernel_cfg->local_work_size,
                                             kernel_cfg->work_dim,
                                             &gpu_time);
    } else {
        run_result = opencl_run_kernel_standard(env, kernel, input_buf, output_buf,
                                               config->src_width, config->src_height,
                                               kernel_cfg->global_work_size,
                                               kernel_cfg->local_work_size,
                                               kernel_cfg->work_dim,
                                               &gpu_time);
    }

    // ... rest of function ...
}
```

### Pros
- ✅ **Simple and explicit** - Clear which function does what
- ✅ **Easy to understand** - No indirection or complex logic
- ✅ **Direct debugging** - Can step directly into specific function
- ✅ **Type-safe** - Compiler catches wrong function calls
- ✅ **No runtime overhead** - Direct function calls

### Cons
- ❌ **Code duplication** - Setup and teardown code repeated
- ❌ **Maintenance burden** - Changes must be applied to all functions
- ❌ **Poor scalability** - N execution modes = N functions
- ❌ **Caller complexity** - Caller must implement selection logic
- ❌ **Rigid design** - Hard to add new modes without adding new functions
- ❌ **Violation of DRY** - Repeated code across functions

### When to Use
- You only have 2-3 execution modes and don't expect more
- You value simplicity and explicitness over flexibility
- Code duplication is acceptable for your use case
- You're prototyping or in early development stages

---

## Comparison Matrix

| Aspect | Config-Driven | Function Pointer | Auto-Detection | Separate Functions |
|--------|---------------|------------------|----------------|-------------------|
| **Complexity** | Low | Medium | High | Very Low |
| **Flexibility** | High | Very High | Low | Low |
| **Maintainability** | High | High | Medium | Low |
| **Scalability** | High | High | Medium | Low |
| **Runtime Overhead** | None | Minimal | Medium | None |
| **Debugging Ease** | High | Medium | Low | High |
| **Config Coupling** | High | Medium | Low | Medium |
| **Code Duplication** | Low | Low | Low | High |
| **User Control** | Explicit | Explicit | Automatic | Explicit |

---

## Recommendation for This Project

**Use Approach 1: Config-Driven API Selection**

### Rationale

1. **Architecture alignment** - This project already uses config-driven design (`config/*.ini`)
2. **Explicit control** - Each kernel variant clearly declares its requirements
3. **Easy maintenance** - Centralized in config files
4. **Good scalability** - Easy to add new execution modes
5. **No runtime overhead** - Decision made at config parse time

### Implementation Steps

1. Add `ExecutionMode` enum to `src/utils/config_parser.h`
2. Add `exec_mode` field to `KernelConfig` struct
3. Update config parser to read `execution_mode` from INI files
4. Modify `opencl_run_kernel()` to accept `ExecutionMode` parameter
5. Add conditional logic at `opencl_utils.c:336` to select API
6. Update all config files to include `execution_mode` field

---

## Future Extensions

Once the base infrastructure is in place, you can easily add:

- **Async execution mode** - Non-blocking kernel launches
- **Tiled execution mode** - Break large images into tiles
- **Batched execution mode** - Process multiple images in one call
- **Profiling mode** - Enhanced instrumentation
- **Vendor-specific modes** - NVIDIA, AMD, Intel optimizations

Each new mode is just:
1. Add enum value
2. Add case in execution logic
3. Update relevant config files

---

## References

- OpenCL Specification 3.0: https://www.khronos.org/registry/OpenCL/
- Config System Documentation: `docs/CONFIG_SYSTEM.md`
- Algorithm Interface: `docs/ALGORITHM_INTERFACE.md`
