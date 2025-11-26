# Algorithm Development Guide

This guide covers the implemented algorithms and how to add new ones to the framework.

## Implemented Algorithms

### 1. Dilate 3x3
Morphological dilation with a 3x3 structuring element.

**Variants:**
- `v0`: Basic implementation
- `v1`: Optimized with local memory tiling

**Usage:**
```bash
./opencl_host dilate3x3 0    # Basic variant
./opencl_host dilate3x3 1    # Optimized variant
```

### 2. Gaussian 5x5
Gaussian blur with a 5x5 kernel.

**Variants:**
- `v0`: Basic implementation

**Usage:**
```bash
./opencl_host gaussian5x5 0
```

## Adding New Algorithms

Adding a new algorithm is now extremely simple with auto-registration. Here's how to add an algorithm (e.g., `erode3x3`):

### 1. Create Directory Structure
```bash
mkdir -p src/erode/c_ref
mkdir -p src/erode/cl
```

### 2. Implement C Reference + Auto-Register
**File:** `src/erode/c_ref/erode3x3_ref.c`

```c
#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/verify.h"
#include <stddef.h>

/* C reference implementation */
void erode3x3_ref(const OpParams* params) {
    if (params == NULL) return;

    unsigned char* input = params->input;
    unsigned char* output = params->output;
    int width = params->src_width;
    int height = params->src_height;

    /* Implement your algorithm here */
    /* This serves as the ground truth for verification */
}

/* Verification function */
static int erode3x3_verify(const OpParams* params, float* max_error) {
    if (params == NULL) return 0;
    return verify_exact_match(params->gpu_output, params->ref_output,
                             params->dst_width, params->dst_height, max_error);
}

/* Auto-register - ONE LINE! */
REGISTER_ALGORITHM(erode3x3, "Erode 3x3", erode3x3_ref, erode3x3_verify)
```

That's it! The algorithm automatically registers itself before `main()` runs.

### 3. Create OpenCL Kernel
**File:** `src/erode/cl/erode0.cl`

```c
__kernel void erode3x3(__global const uchar* input,
                       __global uchar* output,
                       int width,
                       int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    /* Implement your kernel here */
}
```

Add optimized variants as needed (`erode1.cl`, etc.)

### 4. Create Configuration File
**File:** `config/erode3x3.ini`

```ini
# Erode 3x3 Algorithm Configuration
# Note: op_id is auto-derived from filename (erode3x3.ini -> op_id = erode3x3)

[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920
dst_height = 1080

# Variant 0: Basic implementation
[kernel.v0]
kernel_file = src/erode/cl/erode0.cl
kernel_function = erode3x3
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

### 5. Build and Run
```bash
./scripts/build.sh
./build/opencl_host erode3x3 0    # Run by algorithm name!
```

### That's It!

No manual registration, no header files, no modification to `main.c` or existing configs. Just:
1. Implement your algorithm `.c` file with `REGISTER_ALGORITHM()` macro
2. Create your kernel `.cl` file
3. Create your config `erode3x3.ini`
4. Build and run

The `REGISTER_ALGORITHM` macro automatically:
- Creates the `Algorithm` struct
- Registers it before `main()` runs using constructor attributes
- Makes it available by name

## Advanced Topics

### Custom Parameters

For algorithms needing custom parameters (kernel weights, thresholds, etc.):

```c
typedef struct {
    float* kernel_weights;
    int kernel_size;
    float threshold;
} ErodeParams;

void erode_custom_ref(const OpParams* params) {
    ErodeParams* custom = (ErodeParams*)params->algo_params;
    // Use custom->kernel_weights, custom->kernel_size, etc.
}
```

See `ALGORITHM_INTERFACE.md` for details.

### Multiple Variants

To add multiple optimized variants of your algorithm:

**Variant 1: Optimized with local memory**
1. Create `src/erode/cl/erode1.cl` with optimized kernel
2. Add to `config/erode3x3.ini`:
```ini
[kernel.v1]
kernel_file = src/erode/cl/erode1.cl
kernel_function = erode3x3_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

Run with: `./opencl_host erode3x3 1`

### Algorithm Categories

Organize algorithms by category:
```
src/
├── morphology/           # Morphological operations
│   ├── dilate/
│   ├── erode/
│   └── open/
├── filters/              # Image filters
│   ├── gaussian/
│   ├── median/
│   └── bilateral/
└── transforms/           # Image transforms
    ├── resize/
    ├── rotate/
    └── warp/
```

## MISRA-C Compliance Notes

When implementing new algorithms, follow these patterns for compliance:

- Use `const OpParams* params` for all reference implementations
- Check `params == NULL` before use
- Use `verify_exact_match()` or `verify_with_tolerance()` for verification
- Follow existing patterns in `dilate3x3_ref.c` and `gaussian5x5_ref.c`
- Use safe arithmetic from `safe_ops.h` (e.g., `safe_mul_int()`)
- Add bounds checking for array accesses
- Validate all input parameters

## Testing Your Algorithm

After implementing your algorithm:

1. **Generate test input:**
```bash
python3 scripts/generate_test_image.py
```

2. **Build:**
```bash
./scripts/build.sh
```

3. **Run with verification:**
```bash
./build/opencl_host myalgo 0
```

The framework automatically:
- Runs C reference implementation
- Creates golden sample from reference output
- Compiles and runs OpenCL kernel
- Verifies GPU output against reference
- Reports timing and speedup

4. **Check output:**
- Verification should show **PASSED**
- Max error should be acceptable (0.00 for exact, < 1.0 for approximate)
- Speedup shows GPU performance improvement

## Common Patterns

### Image Filters
```c
void filter_ref(const OpParams* params) {
    // Extract parameters
    unsigned char* input = params->input;
    unsigned char* output = params->output;
    int width = params->src_width;
    int height = params->src_height;

    // Apply filter with border handling
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Compute filtered value
            output[y * width + x] = compute_filter(input, x, y, width, height);
        }
    }
}
```

### Morphological Operations
```c
void morph_ref(const OpParams* params) {
    // Use border_mode from params for edge handling
    BorderMode mode = params->border_mode;

    // Iterate over structuring element
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            // Apply operation
        }
    }
}
```

### Resize Operations
```c
void resize_ref(const OpParams* params) {
    // Different source and destination sizes
    int src_w = params->src_width;
    int src_h = params->src_height;
    int dst_w = params->dst_width;
    int dst_h = params->dst_height;

    // Compute scaling factors
    float scale_x = (float)src_w / dst_w;
    float scale_y = (float)src_h / dst_h;

    // Sample and interpolate
}
```

## Related Documentation

- **[ALGORITHM_INTERFACE.md](ALGORITHM_INTERFACE.md)** - Detailed OpParams interface guide
- **[CONFIG_SYSTEM.md](CONFIG_SYSTEM.md)** - Configuration system documentation
- **[../README.md](../README.md)** - Main project documentation
