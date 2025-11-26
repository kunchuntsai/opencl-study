# Algorithm Interface Guide

## Overview

The flexible `OpParams` interface supports algorithms with varying parameter requirements while maintaining a consistent API.

## OpParams Structure

```c
typedef struct {
    /* Input image */
    unsigned char* input;
    int src_width, src_height;
    int src_stride;  /* 0 = packed */

    /* Output image */
    unsigned char* output;
    int dst_width, dst_height;
    int dst_stride;  /* 0 = packed */

    /* Verification buffers */
    unsigned char* ref_output;
    unsigned char* gpu_output;

    /* Border handling */
    BorderMode border_mode;
    unsigned char border_value;

    /* Algorithm-specific extension */
    void* algo_params;
    size_t algo_params_size;
} OpParams;
```

## Common Use Cases

### 1. Simple Filters (Dilate, Erode)
Uses: `input`, `output`, `src_width`, `src_height`, `border_mode`

```c
void dilate3x3_ref(const OpParams* params) {
    unsigned char* input = params->input;
    unsigned char* output = params->output;
    int width = params->src_width;
    int height = params->src_height;

    // Implementation...
}
```

### 2. Resize Operations
Uses: Different `src_*` and `dst_*` dimensions

```c
void resize_ref(const OpParams* params) {
    int src_w = params->src_width;
    int src_h = params->src_height;
    int dst_w = params->dst_width;
    int dst_h = params->dst_height;

    // Scale image from src to dst dimensions
}
```

### 3. Convolution with Custom Kernel
Uses: `algo_params` for kernel weights

```c
typedef struct {
    float* kernel;    // Kernel weights
    int kernel_size;  // e.g., 3 for 3x3, 5 for 5x5
} ConvolutionParams;

void custom_conv_ref(const OpParams* params) {
    ConvolutionParams* conv = (ConvolutionParams*)params->algo_params;
    float* kernel = conv->kernel;
    int ksize = conv->kernel_size;

    // Use custom kernel for convolution
}

// Usage in main.c:
ConvolutionParams conv_params = {
    .kernel = my_kernel_weights,
    .kernel_size = 7
};
OpParams op_params = {
    .input = input_buffer,
    .output = output_buffer,
    .src_width = 1920,
    .src_height = 1080,
    .algo_params = &conv_params,
    .algo_params_size = sizeof(ConvolutionParams)
};
algo->reference_impl(&op_params);
```

### 4. Separable Filters
Uses: `algo_params` for separate X and Y kernels

```c
typedef struct {
    float* kernel_x;
    float* kernel_y;
    int kernel_size;
} SeparableKernelParams;

void separable_filter_ref(const OpParams* params) {
    SeparableKernelParams* sep = (SeparableKernelParams*)params->algo_params;

    // Apply horizontal kernel
    // Apply vertical kernel
}
```

## Adding a New Algorithm

### Step 1: Implement Reference Function

```c
void myalgo_ref(const OpParams* params) {
    // Extract needed parameters
    unsigned char* input = params->input;
    unsigned char* output = params->output;
    int width = params->src_width;
    int height = params->src_height;

    // Extract algorithm-specific params if needed
    if (params->algo_params != NULL) {
        MyAlgoParams* my_params = (MyAlgoParams*)params->algo_params;
        // Use custom parameters
    }

    // Implement algorithm
}
```

### Step 2: Implement Verify Function

```c
static int myalgo_verify(const OpParams* params, float* max_error) {
    if (params == NULL) {
        return 0;
    }
    return verify_exact_match(
        params->gpu_output,
        params->ref_output,
        params->dst_width,
        params->dst_height,
        max_error
    );
}
```

### Step 3: Register Algorithm

```c
REGISTER_ALGORITHM(myalgo, "My Algorithm", myalgo_ref, myalgo_verify)
```

### Step 4: Configure in config.ini

```ini
[image]
op_id = myalgo

[kernel.v0]
kernel_file = src/myalgo/cl/myalgo.cl
kernel_function = myalgo_kernel
work_dim = 2
global_work_size = 1920,1080
local_work_size = 16,16
```

## Benefits

1. **Flexible**: Supports algorithms with different parameter needs
2. **Extensible**: Add algorithm-specific params via `algo_params`
3. **Consistent**: Single interface for all algorithms
4. **Type-safe**: Compiler checks struct initialization
5. **Future-proof**: Easy to add new common parameters

## Configuration Terminology

**IMPORTANT**: The `op_id` in your code must match the `op_id` in config.ini:

```c
// Code
REGISTER_ALGORITHM(dilate3x3, "Dilate 3x3", dilate3x3_ref, dilate3x3_verify)
```

```ini
# Config
op_id = dilate3x3
```

This consistency allows the framework to:
- Find the correct algorithm at runtime
- Load the right kernel configurations
- Cache results per algorithm
