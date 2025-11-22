# OpenCL Multi-Kernel Host Implementation Plan

## Architecture Overview

The system will use a modular, plugin-like architecture where each algorithm is self-contained with its OpenCL kernels, C reference implementation, and verification logic.

---

## Directory Structure

```
project/
├── config/
|   └── config.ini                      # Runtime configuration
├── src/
│   ├── main.c                      # Entry point, menu system
│   ├── Makefile
│   ├── dilate/
│   |   ├── c_ref
│   |   └── cl/
│   │      ├── dilate_0.cl
│   |      └── dilate_1.cl
│   ├── gaussian/
│   |   ├── c_ref
│   |   └── cl/
│   └── utils/
│       ├── algorithm_interface.h   # Common interface for all algorithms
│       ├── algorithm_registry.c/.h     # Algorithm registration and dispatch
│       ├── opencl_utils.c/.h           # OpenCL initialization, device selection
│       ├── config_parser.c/.h          # INI file parser
│       └── image_io.c/.h               # Raw image read/write
├── scripts/
│   ├── build.sh
│   └── run.sh
└── test_data/
    ├── dilate/ # Input image, output image, golden
    └── gaussian/
```

---

## Core Components

### 1. **algorithm_interface.h**
Defines the common interface that all algorithms must implement:

```c
typedef struct {
    char name[64];              // Algorithm display name (e.g., "Dilate 3x3")
    char id[32];                // Algorithm ID (e.g., "dilate3x3")
    
    // Function pointers
    void (*reference_impl)(unsigned char* input, unsigned char* output, 
                          int width, int height);
    
    int (*verify_result)(unsigned char* gpu_output, unsigned char* ref_output,
                        int width, int height, float* max_error);
    
    void (*print_info)(void);   // Optional: print algorithm-specific info
} Algorithm;
```

### 2. **algorithm_registry.c/.h**
- Maintains a registry of all available algorithms
- Provides lookup by algorithm ID
- Auto-registration mechanism

```c
// Registration function (called at startup)
void register_algorithm(Algorithm* algo);

// Lookup function
Algorithm* find_algorithm(const char* algo_id);

// List all available algorithms
void list_algorithms(void);
```

### 3. **config_parser.c/.h**
Parses INI file and extracts configuration for each kernel variant:

```c
typedef struct {
    char algo_id[32];           // e.g., "dilate3x3"
    char variant_id[32];        // e.g., "v0", "v1"
    char kernel_file[256];
    char kernel_function[64];
    int work_dim;
    size_t global_work_size[3];
    size_t local_work_size[3];
} KernelConfig;

typedef struct {
    char input_image[256];
    char output_image[256];
    int width;
    int height;
    int num_kernels;
    KernelConfig kernels[32];   // Support up to 32 kernel variants
} Config;

// Parse the entire config file
int parse_config(const char* filename, Config* config);

// Get all variants for a specific algorithm
int get_algorithm_variants(Config* config, const char* algo_id, 
                          KernelConfig** variants, int* count);
```

### 4. **opencl_utils.c/.h**
OpenCL boilerplate code:

```c
typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
} OpenCLEnv;

// Initialize OpenCL
int opencl_init(OpenCLEnv* env);

// Load and compile kernel from file
cl_kernel opencl_load_kernel(OpenCLEnv* env, const char* kernel_file, 
                            const char* kernel_name);

// Execute kernel
int opencl_execute_kernel(OpenCLEnv* env, cl_kernel kernel,
                         cl_mem input_buf, cl_mem output_buf,
                         int work_dim, size_t* global_size, size_t* local_size);

// Cleanup
void opencl_cleanup(OpenCLEnv* env);
```

### 5. **image_io.c/.h**
Simple raw grayscale image I/O:

```c
// Read raw grayscale image
unsigned char* read_image(const char* filename, int width, int height);

// Write raw grayscale image
int write_image(const char* filename, unsigned char* data, int width, int height);
```

### 6. **main.c**
Main program flow:

```c
1. Parse config.ini
2. Initialize OpenCL
3. Register all algorithms (dilate3x3, gaussian5x5, etc.)
4. Display menu of available algorithms
5. User selects algorithm
6. Display variants for selected algorithm (v0, v1, v2...)
7. User selects variant
8. Load input image
9. Run C reference implementation
10. Run OpenCL kernel
11. Verify results
12. Display statistics (execution time, errors)
13. Save output image
14. Return to menu or exit
```

---

## Algorithm Implementations

### Dilate 3x3 Algorithm

**File: `src/algorithms/dilate3x3.c`**

```c
#include "algorithm_interface.h"
#include "../../reference/dilate3x3_ref.h"

// Reference implementation (calls the reference code)
void dilate3x3_reference(unsigned char* input, unsigned char* output, 
                        int width, int height) {
    dilate3x3_ref(input, output, width, height);
}

// Verification: check if GPU and reference outputs match
int dilate3x3_verify(unsigned char* gpu_output, unsigned char* ref_output,
                    int width, int height, float* max_error) {
    int errors = 0;
    *max_error = 0.0f;
    
    for (int i = 0; i < width * height; i++) {
        float diff = fabs((float)gpu_output[i] - (float)ref_output[i]);
        if (diff > *max_error) {
            *max_error = diff;
        }
        if (diff > 0.0f) {
            errors++;
        }
    }
    
    return (errors == 0); // Return 1 if passed, 0 if failed
}

void dilate3x3_print_info(void) {
    printf("Dilate 3x3 - Morphological dilation with 3x3 structuring element\n");
}

// Algorithm registration (called at startup)
Algorithm dilate3x3_algorithm = {
    .name = "Dilate 3x3",
    .id = "dilate3x3",
    .reference_impl = dilate3x3_reference,
    .verify_result = dilate3x3_verify,
    .print_info = dilate3x3_print_info
};
```

### Gaussian 5x5 Algorithm

**File: `src/algorithms/gaussian5x5.c`**

Similar structure to dilate3x3, but implements Gaussian blur verification.

```c
// Verification with tolerance for floating-point differences
int gaussian5x5_verify(unsigned char* gpu_output, unsigned char* ref_output,
                      int width, int height, float* max_error) {
    int errors = 0;
    *max_error = 0.0f;
    float tolerance = 1.0f; // Allow 1 intensity level difference due to rounding
    
    for (int i = 0; i < width * height; i++) {
        float diff = fabs((float)gpu_output[i] - (float)ref_output[i]);
        if (diff > *max_error) {
            *max_error = diff;
        }
        if (diff > tolerance) {
            errors++;
        }
    }
    
    float error_rate = (float)errors / (width * height) * 100.0f;
    return (error_rate < 0.1f); // Pass if less than 0.1% pixels differ
}
```

---

## Reference Implementations

### Dilate 3x3 Reference

**File: `reference/dilate3x3_ref.c`**

```c
void dilate3x3_ref(unsigned char* input, unsigned char* output, 
                   int width, int height) {
    // Handle borders by replication
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char max_val = 0;
            
            // 3x3 window
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    
                    // Clamp to borders
                    ny = (ny < 0) ? 0 : (ny >= height) ? height - 1 : ny;
                    nx = (nx < 0) ? 0 : (nx >= width) ? width - 1 : nx;
                    
                    unsigned char val = input[ny * width + nx];
                    if (val > max_val) {
                        max_val = val;
                    }
                }
            }
            
            output[y * width + x] = max_val;
        }
    }
}
```

### Gaussian 5x5 Reference

**File: `reference/gaussian5x5_ref.c`**

```c
void gaussian5x5_ref(unsigned char* input, unsigned char* output, 
                     int width, int height) {
    // Gaussian 5x5 kernel (normalized)
    float kernel[5][5] = {
        {1, 4, 6, 4, 1},
        {4, 16, 24, 16, 4},
        {6, 24, 36, 24, 6},
        {4, 16, 24, 16, 4},
        {1, 4, 6, 4, 1}
    };
    float kernel_sum = 256.0f;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            
            // 5x5 window
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    
                    // Clamp to borders
                    ny = (ny < 0) ? 0 : (ny >= height) ? height - 1 : ny;
                    nx = (nx < 0) ? 0 : (nx >= width) ? width - 1 : nx;
                    
                    float weight = kernel[dy + 2][dx + 2];
                    sum += input[ny * width + nx] * weight;
                }
            }
            
            output[y * width + x] = (unsigned char)(sum / kernel_sum + 0.5f);
        }
    }
}
```

---

## OpenCL Kernel Examples

### Dilate Kernel Variant 0 (Basic)

**File: `kernels/dilate0.cl`**

```opencl
__kernel void dilate3x3(__global const uchar* input,
                        __global uchar* output,
                        int width,
                        int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if (x >= width || y >= height) return;
    
    uchar max_val = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);
            
            uchar val = input[ny * width + nx];
            max_val = max(max_val, val);
        }
    }
    
    output[y * width + x] = max_val;
}
```

### Dilate Kernel Variant 1 (Optimized with Local Memory)

**File: `kernels/dilate1.cl`**

```opencl
__kernel void dilate3x3_optimized(__global const uchar* input,
                                  __global uchar* output,
                                  int width,
                                  int height) {
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int lx = get_local_id(0);
    int ly = get_local_id(1);
    
    // Local memory tile with halo
    __local uchar tile[18][18]; // 16x16 + 2 pixel border
    
    // Load tile (including halo)
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int tx = lx + dx + 1;
            int ty = ly + dy + 1;
            int gx_src = clamp(gx + dx, 0, width - 1);
            int gy_src = clamp(gy + dy, 0, height - 1);
            
            if (dx == 0 && dy == 0) {
                tile[ty][tx] = input[gy_src * width + gx_src];
            }
        }
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (gx >= width || gy >= height) return;
    
    uchar max_val = 0;
    for (int dy = 0; dy <= 2; dy++) {
        for (int dx = 0; dx <= 2; dx++) {
            uchar val = tile[ly + dy][lx + dx];
            max_val = max(max_val, val);
        }
    }
    
    output[gy * width + gx] = max_val;
}
```

### Gaussian Kernel Variant 0 (Basic)

**File: `kernels/gaussian0.cl`**

```opencl
__kernel void gaussian5x5(__global const uchar* input,
                          __global uchar* output,
                          int width,
                          int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    
    if (x >= width || y >= height) return;
    
    // Gaussian 5x5 kernel
    float kernel[25] = {
        1, 4, 6, 4, 1,
        4, 16, 24, 16, 4,
        6, 24, 36, 24, 6,
        4, 16, 24, 16, 4,
        1, 4, 6, 4, 1
    };
    
    float sum = 0.0f;
    int idx = 0;
    
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);
            
            float weight = kernel[idx++];
            sum += input[ny * width + nx] * weight;
        }
    }
    
    output[y * width + x] = convert_uchar_sat(sum / 256.0f);
}
```

---

## Main Program Flow

**File: `src/main.c`**

```c
int main(int argc, char** argv) {
    Config config;
    OpenCLEnv env;
    
    // 1. Parse configuration
    if (parse_config("config.ini", &config) != 0) {
        fprintf(stderr, "Failed to parse config.ini\n");
        return 1;
    }
    
    // 2. Initialize OpenCL
    if (opencl_init(&env) != 0) {
        fprintf(stderr, "Failed to initialize OpenCL\n");
        return 1;
    }
    
    // 3. Register algorithms
    register_algorithm(&dilate3x3_algorithm);
    register_algorithm(&gaussian5x5_algorithm);
    
    // 4. Main menu loop
    while (1) {
        printf("\n=== OpenCL Image Processing Framework ===\n");
        list_algorithms();
        printf("0. Exit\n");
        printf("Select algorithm: ");
        
        int choice;
        scanf("%d", &choice);
        
        if (choice == 0) break;
        
        Algorithm* algo = get_algorithm_by_index(choice - 1);
        if (!algo) {
            printf("Invalid selection\n");
            continue;
        }
        
        // 5. Show kernel variants for selected algorithm
        KernelConfig* variants;
        int variant_count;
        get_algorithm_variants(&config, algo->id, &variants, &variant_count);
        
        printf("\nAvailable variants for %s:\n", algo->name);
        for (int i = 0; i < variant_count; i++) {
            printf("%d. %s (%s)\n", i + 1, variants[i].variant_id, 
                   variants[i].kernel_file);
        }
        
        printf("Select variant: ");
        int variant_choice;
        scanf("%d", &variant_choice);
        
        if (variant_choice < 1 || variant_choice > variant_count) {
            printf("Invalid variant\n");
            continue;
        }
        
        KernelConfig* selected = &variants[variant_choice - 1];
        
        // 6. Run algorithm
        run_algorithm(algo, selected, &config, &env);
    }
    
    // Cleanup
    opencl_cleanup(&env);
    return 0;
}

void run_algorithm(Algorithm* algo, KernelConfig* kernel_cfg, 
                   Config* config, OpenCLEnv* env) {
    // Load input image
    unsigned char* input = read_image(config->input_image, 
                                     config->width, config->height);
    int img_size = config->width * config->height;
    
    // Allocate output buffers
    unsigned char* gpu_output = malloc(img_size);
    unsigned char* ref_output = malloc(img_size);
    
    // Run C reference
    printf("Running C reference implementation...\n");
    clock_t ref_start = clock();
    algo->reference_impl(input, ref_output, config->width, config->height);
    clock_t ref_end = clock();
    double ref_time = (double)(ref_end - ref_start) / CLOCKS_PER_SEC * 1000;
    
    // Run OpenCL kernel
    printf("Running OpenCL kernel: %s...\n", kernel_cfg->kernel_function);
    cl_kernel kernel = opencl_load_kernel(env, kernel_cfg->kernel_file, 
                                         kernel_cfg->kernel_function);
    
    // Create buffers
    cl_mem input_buf = clCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      img_size, input, NULL);
    cl_mem output_buf = clCreateBuffer(env->context, CL_MEM_WRITE_ONLY,
                                       img_size, NULL, NULL);
    
    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
    clSetKernelArg(kernel, 2, sizeof(int), &config->width);
    clSetKernelArg(kernel, 3, sizeof(int), &config->height);
    
    // Execute kernel
    cl_event event;
    clEnqueueNDRangeKernel(env->queue, kernel, kernel_cfg->work_dim,
                          NULL, kernel_cfg->global_work_size,
                          kernel_cfg->local_work_size, 0, NULL, &event);
    clFinish(env->queue);
    
    // Get execution time
    cl_ulong time_start, time_end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, 
                           sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, 
                           sizeof(time_end), &time_end, NULL);
    double gpu_time = (time_end - time_start) / 1000000.0; // Convert to ms
    
    // Read back results
    clEnqueueReadBuffer(env->queue, output_buf, CL_TRUE, 0, 
                       img_size, gpu_output, 0, NULL, NULL);
    
    // Verify results
    float max_error;
    int passed = algo->verify_result(gpu_output, ref_output, 
                                    config->width, config->height, &max_error);
    
    // Display results
    printf("\n=== Results ===\n");
    printf("C Reference time: %.3f ms\n", ref_time);
    printf("OpenCL GPU time:  %.3f ms\n", gpu_time);
    printf("Speedup:          %.2fx\n", ref_time / gpu_time);
    printf("Verification:     %s\n", passed ? "PASSED" : "FAILED");
    printf("Max error:        %.2f\n", max_error);
    
    // Save output
    write_image(config->output_image, gpu_output, config->width, config->height);
    printf("Output saved to: %s\n", config->output_image);
    
    // Cleanup
    free(input);
    free(gpu_output);
    free(ref_output);
    clReleaseMemObject(input_buf);
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseEvent(event);
}
```

---

## Build System

**File: `Makefile`**

```makefile
CC = gcc
CFLAGS = -Wall -O2 -std=c99
LDFLAGS = -lOpenCL -lm

SRC_DIR = src
REF_DIR = reference
OBJ_DIR = build

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/opencl_utils.c \
       $(SRC_DIR)/config_parser.c \
       $(SRC_DIR)/image_io.c \
       $(SRC_DIR)/algorithm_registry.c \
       $(SRC_DIR)/algorithms/dilate3x3.c \
       $(SRC_DIR)/algorithms/gaussian5x5.c \
       $(REF_DIR)/dilate3x3_ref.c \
       $(REF_DIR)/gaussian5x5_ref.c

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

TARGET = opencl_host

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
```

---

## Usage Example

```bash
# Build the project
make

# Run the host executable
./opencl_host

# Menu will appear:
=== OpenCL Image Processing Framework ===
1. Dilate 3x3
2. Gaussian 5x5
0. Exit
Select algorithm: 1

Available variants for Dilate 3x3:
1. v0 (kernels/dilate0.cl)
2. v1 (kernels/dilate1.cl)
Select variant: 1

Running C reference implementation...
Running OpenCL kernel: dilate3x3...

=== Results ===
C Reference time: 125.340 ms
OpenCL GPU time:  3.245 ms
Speedup:          38.63x
Verification:     PASSED
Max error:        0.00
Output saved to: output.raw
```

---

## Adding New Algorithms

To add a new algorithm (e.g., `erode3x3`):

1. Create reference implementation in `reference/erode3x3_ref.c`
2. Create algorithm wrapper in `src/algorithms/erode3x3.c`
3. Register in `main.c`: `register_algorithm(&erode3x3_algorithm);`
4. Add kernel variants in `kernels/erode0.cl`, `erode1.cl`, etc.
5. Update `config.ini` with new sections for each variant

---

## Key Design Decisions

1. **Separation of Concerns**: Each algorithm is self-contained with its own reference, verification, and metadata
2. **Config-Driven**: All runtime parameters in config.ini, no recompilation needed
3. **Extensible**: Easy to add new algorithms by following the interface pattern
4. **Verification Built-in**: Each algorithm defines its own pass/fail criteria
5. **Multiple Variants**: Support multiple kernel implementations per algorithm for benchmarking
6. **Simple I/O**: Raw grayscale format for simplicity (can be extended)

---

## Future Enhancements

- Support for other image formats (PNG, JPG via stb_image)
- Multi-platform device selection (CPU vs GPU)
- Batch processing mode
- Performance profiling with multiple runs
- Automatic test suite running all algorithms
- JSON output for automated testing
- GUI frontend (optional)