# OpenCL Image Processing Framework - Codebase Study

## Project Overview

This is a modular OpenCL-based image processing framework written in C that demonstrates GPU-accelerated image operations with a focus on code quality, extensibility, and MISRA-C:2023 compliance (100%).

**Key Characteristics:**
- **Language:** C99 with OpenCL
- **Platform:** macOS (uses Apple's OpenCL framework)
- **Architecture:** Plugin-style design with pluggable algorithms
- **Compliance:** 100% MISRA-C:2023 compliant
- **Current Branch:** misra_c

---

## Directory Structure

```
opencl-study/
├── build/                  # Compiled artifacts
│   ├── obj/                # Object files (.o) organized by algorithm
│   └── opencl_host         # Main executable (54KB)
├── config/                 # Runtime configuration
│   └── config.ini          # Algorithm and kernel configurations
├── src/                    # Source code
│   ├── main.c              # Entry point and orchestration
│   ├── Makefile            # Build configuration
│   ├── dilate/             # Dilate 3x3 algorithm module
│   ├── gaussian/           # Gaussian 5x5 blur module
│   └── utils/              # Shared utilities and infrastructure
├── scripts/                # Build and utility scripts
│   ├── build.sh            # Build automation
│   ├── run.sh              # Runtime automation
│   └── generate_test_image.py  # Test image generator
└── test_data/              # Test images and results
```

---

## Core Modules

### 1. Main Entry Point (`src/main.c`)

**Purpose:** Application orchestration, user interface, and execution control

**Key Responsibilities:**
- Parse configuration from `config/config.ini`
- Initialize OpenCL environment (platform, device, context, queue)
- Register all available algorithms
- Present interactive menu for algorithm/variant selection
- Execute both C reference and OpenCL implementations
- Verify results and report performance metrics
- Manage static buffers (MISRA-C compliant - no dynamic allocation)

**Execution Flow:**
```
1. Load configuration → 2. Initialize OpenCL → 3. Register algorithms
→ 4. Display menu → 5. User selects algorithm/variant
→ 6. Load input image → 7. Run C reference (with timing)
→ 8. Run OpenCL kernel (with profiling) → 9. Verify results
→ 10. Report metrics → 11. Save output → 12. Cleanup
```

**Static Buffers:**
- `gpu_output_buffer[16MB]` - GPU kernel output
- `ref_output_buffer[16MB]` - C reference output

---

### 2. Algorithm Interface & Registry (`src/utils/`)

#### op_interface.h - Common Algorithm Interface

Defines the standard interface that all algorithms must implement:

```c
typedef struct {
    char name[64];              /* Display name: "Dilate 3x3" */
    char id[32];                /* Algorithm ID: "dilate3x3" */
    void (*reference_impl)(...);/* C reference implementation */
    int (*verify_result)(...);  /* Verification function */
    void (*print_info)(void);   /* Algorithm info display */
} Algorithm;
```

#### op_registry.c/.h - Algorithm Registration System

**Purpose:** Central registry for all algorithms (supports up to 32 algorithms)

**Key Functions:**
- `register_algorithm()` - Add algorithm to registry at startup
- `find_algorithm()` - Lookup algorithm by ID string
- `list_algorithms()` - Display all registered algorithms in menu
- `get_algorithm_by_index()` - Access by numeric index

This design enables a plugin-like architecture where new algorithms can be added without modifying existing code.

---

### 3. Configuration System (`src/utils/config_parser.c/.h`)

**Purpose:** Parse and manage runtime configuration from INI files

**Configuration Structure:**
```ini
[image]
input = test_data/input.bin
output = test_data/output.bin
width = 1024
height = 1024

[kernel.dilate_v0]
op_id = dilate3x3
variant_id = v0
kernel_file = src/dilate/cl/dilate0.cl
kernel_function = dilate3x3
work_dim = 2
global_work_size = 1024,1024
local_work_size = 16,16
```

**Key Features:**
- Thread-safe parsing using `strtok_r()` (MISRA-C compliant)
- Safe integer conversion with overflow checking
- Support for multiple kernel variants per algorithm
- Dynamic variant discovery for menu display

**Data Structure:**
```c
typedef struct {
    char op_id[32];
    char variant_id[32];
    char kernel_file[256];
    char kernel_function[64];
    int work_dim;
    size_t global_work_size[3];
    size_t local_work_size[3];
} KernelConfig;
```

---

### 4. OpenCL Infrastructure (`src/utils/opencl_utils.c/.h`)

**Purpose:** Manage OpenCL platform, device, context, and kernel execution

**Key Components:**

**OpenCL Environment:**
```c
typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
} OpenCLEnv;
```

**Key Functions:**
- `opencl_init()` - Initialize OpenCL (prefers GPU, falls back to CPU)
- `opencl_load_kernel()` - Load and compile kernel from file
- `opencl_cleanup()` - Release all OpenCL resources

**Features:**
- Automatic device selection (GPU priority)
- Profiling support enabled (`CL_QUEUE_PROFILING_ENABLE`)
- Static buffer for kernel source code (1MB max)
- Comprehensive error handling with cleanup
- Build log capture for compilation errors

---

### 5. Image I/O System (`src/utils/image_io.c/.h`)

**Purpose:** Read and write raw grayscale images

**Image Format:**
- Type: Raw binary (8-bit unsigned grayscale)
- Layout: Row-major order
- Max size: 4096×4096 pixels (16MB)

**Key Functions:**
- `read_image()` - Load image from binary file into static buffer
- `write_image()` - Save image to binary file from buffer

**Safety Features:**
- Safe integer overflow checking for size calculations
- Static buffer allocation (MISRA-C compliant)
- File error handling with informative messages

---

### 6. Safe Operations (`src/utils/safe_ops.h`)

**Purpose:** MISRA-C compliant arithmetic operations with overflow protection

**Key Functions:**
- `safe_mul_int()` - Integer multiplication with overflow detection
- `safe_mul_size()` - Size_t multiplication with overflow checking
- `safe_add_size()` - Size_t addition with overflow checking
- `safe_strtol()` - Safe string-to-long conversion
- `safe_str_to_size()` - Safe string-to-size_t conversion

These inline functions prevent undefined behavior from arithmetic overflow, a requirement for MISRA-C:2023 compliance.

---

## Algorithm Modules

### 7. Dilate 3x3 Module (`src/dilate/`)

**Purpose:** Morphological dilation with 3×3 structuring element (finds local maximum in neighborhood)

**Module Structure:**
```
dilate/
├── dilate3x3.c/.h          # Algorithm wrapper and registration
├── c_ref/
│   └── dilate3x3_ref.c/.h  # C reference implementation
└── cl/
    ├── dilate0.cl          # Basic OpenCL kernel
    └── dilate1.cl          # Optimized kernel with local memory
```

**Algorithm Wrapper (dilate3x3.c):**
- Implements Algorithm interface
- Registers with `register_algorithm()`
- Provides verification function (exact match required)

**C Reference Implementation:**
- Serial CPU implementation for verification
- Bounds-safe pixel access with border replication
- 3×3 neighborhood max value extraction

**OpenCL Variants:**

**dilate0.cl (Basic):**
- Simple global memory access
- Direct 3×3 neighborhood processing
- Clamp-based boundary handling
- ~9 global memory reads per output pixel

**dilate1.cl (Optimized):**
- Local memory tiling (18×18 with 2-pixel halo)
- Sophisticated halo loading strategy:
  - Center pixels loaded by all threads
  - Edge halos loaded by boundary threads
  - Corner halos loaded cooperatively
- Barrier synchronization
- Significantly reduced global memory bandwidth

---

### 8. Gaussian 5x5 Module (`src/gaussian/`)

**Purpose:** Gaussian blur using 5×5 convolution kernel for smoothing and noise reduction

**Module Structure:**
```
gaussian/
├── gaussian5x5.c/.h        # Algorithm wrapper and registration
├── c_ref/
│   └── gaussian5x5_ref.c/.h  # C reference implementation
└── cl/
    └── gaussian0.cl        # OpenCL kernel
```

**Gaussian Kernel (normalized by 256):**
```
 1  4  6  4  1
 4 16 24 16  4
 6 24 36 24  6
 4 16 24 16  4
 1  4  6  4  1
```

**Algorithm Wrapper:**
- Implements Algorithm interface
- Verification with 1-pixel tolerance (accounts for floating-point rounding)

**C Reference Implementation:**
- Serial CPU implementation
- 5×5 convolution with border replication
- Floating-point accumulation for precision
- Conversion to unsigned char with proper rounding

**OpenCL Kernel (gaussian0.cl):**
- Inline kernel coefficient definition
- 25 global memory reads per output pixel
- Floating-point accumulation
- Saturating conversion to byte output

---

## How the System Works

### System Initialization

1. **Configuration Loading** (`src/main.c:main()`)
   - Parse `config/config.ini`
   - Extract image specifications (path, dimensions)
   - Load kernel configurations for all variants

2. **OpenCL Initialization** (`src/utils/opencl_utils.c:opencl_init()`)
   - Discover OpenCL platforms
   - Select device (GPU preferred, CPU fallback)
   - Create context and command queue with profiling enabled

3. **Algorithm Registration** (`src/main.c:main()`)
   - Call `register_algorithm()` for each algorithm module
   - Algorithms self-register into central registry
   - Enables dynamic menu generation

### User Interaction Flow

1. **Algorithm Selection**
   - Display menu with all registered algorithms
   - User selects algorithm (e.g., "Dilate 3x3")

2. **Variant Selection**
   - Query config for available variants
   - Display variant menu (v0, v1, etc.)
   - User selects kernel variant

3. **Image Loading**
   - Read input image from configured path
   - Load into static buffer (max 4096×4096)

### Execution Pipeline

1. **C Reference Execution** (`algorithm->reference_impl()`)
   - Run CPU-based reference implementation
   - Measure execution time using `clock()`
   - Store results in `ref_output_buffer`

2. **OpenCL Kernel Execution**
   - Load kernel source from file
   - Compile OpenCL kernel for selected device
   - Allocate GPU buffers (input/output)
   - Transfer input data to device
   - Set kernel arguments
   - Enqueue kernel with work sizes from config
   - Enqueue profiling events
   - Wait for completion
   - Read back results to `gpu_output_buffer`
   - Calculate GPU execution time

3. **Verification** (`algorithm->verify_result()`)
   - Compare GPU output vs C reference
   - Use algorithm-specific tolerance (exact or 1-pixel)
   - Report pass/fail status

4. **Performance Reporting**
   - Display C reference time
   - Display GPU kernel time
   - Calculate and display speedup ratio

5. **Output Saving**
   - Write GPU output to configured path
   - Save as raw binary image

### Resource Cleanup

- Release OpenCL buffers
- Release kernel
- Return to menu or exit
- On exit: cleanup OpenCL environment

---

## Build System

### Makefile (`src/Makefile`)

**Compiler Configuration:**
```makefile
CC = gcc
CFLAGS = -std=c99 -O2 -Wall -I.
LDFLAGS = -framework OpenCL -lm  # macOS
```

**Source Discovery:**
- Automatic discovery of all `.c` files in:
  - `main.c`
  - `utils/*.c`
  - `*/dilate/*.c` and `dilate/c_ref/*.c`
  - `gaussian/*.c` and `gaussian/c_ref/*.c`

**Build Targets:**
- `all` - Compile executable
- `run` - Build and execute from project root
- `clean` - Remove build artifacts

**Build Output:**
- Executable: `build/opencl_host` (54KB)
- Object files: `build/obj/` (organized by module)

### Build Scripts

- `scripts/build.sh` - Automated build wrapper
- `scripts/run.sh` - Automated execution from project root

---

## MISRA-C:2023 Compliance

The codebase recently underwent refactoring to achieve 100% MISRA-C:2023 compliance:

### Key Compliance Changes

1. **Static Memory Allocation (Rule 21.3)**
   - Eliminated all `malloc()` calls
   - All buffers are static or stack-allocated
   - Maximum sizes defined at compile time

2. **Safe Arithmetic (Rule 1.3)**
   - Created `safe_ops.h` with overflow checking
   - All size calculations validated before use
   - Safe string-to-integer conversion functions

3. **Thread Safety (Rule 21.17)**
   - Replaced `strtok()` with reentrant `strtok_r()`
   - Removed static variables from functions

4. **Error Handling (Rule 17.7)**
   - All function return values checked
   - Proper cleanup on all error paths
   - Intentionally discarded returns wrapped in `(void)`

5. **Code Clarity (Rules 11.9, 15.6)**
   - Explicit NULL comparisons
   - Braces around all control blocks
   - C-style comments throughout

6. **Const Correctness (Rule 8.13)**
   - Read-only parameters marked `const`

7. **Internal Linkage (Rule 8.7)**
   - Global variables marked `static` where appropriate

---

## Extensibility - Adding New Algorithms

The architecture supports easy addition of new algorithms:

### Step-by-Step Process

1. **Create Module Directory:**
   ```
   mkdir -p src/newalgo/c_ref
   mkdir -p src/newalgo/cl
   ```

2. **Implement C Reference:**
   - Create `src/newalgo/c_ref/newalgo_ref.c/.h`
   - Implement reference algorithm

3. **Create Algorithm Wrapper:**
   - Create `src/newalgo/newalgo.c/.h`
   - Define `Algorithm` struct
   - Implement verification function

4. **Create OpenCL Kernels:**
   - Create `src/newalgo/cl/newalgo0.cl` (basic variant)
   - Add optimized variants as needed

5. **Update Build System:**
   - Makefile automatically discovers new `.c` files

6. **Register Algorithm:**
   - Add registration call in `src/main.c:main()`
   ```c
   register_algorithm(&newalgo_algorithm);
   ```

7. **Add Configuration:**
   - Add kernel config sections to `config/config.ini`
   ```ini
   [kernel.newalgo_v0]
   op_id = newalgo
   variant_id = v0
   kernel_file = src/newalgo/cl/newalgo0.cl
   kernel_function = newalgo_kernel
   work_dim = 2
   global_work_size = 1024,1024
   local_work_size = 16,16
   ```

8. **Create Test Data:**
   - Create `test_data/newalgo/` directory
   - Add test images

The algorithm will automatically appear in the menu system with all configured variants.

---

## Memory Architecture

### Static Buffers (Stack/BSS Allocation)

**In main.c:**
- `gpu_output_buffer[16MB]` - GPU kernel results
- `ref_output_buffer[16MB]` - C reference results

**In image_io.c:**
- `image_buffer[16MB]` - Image I/O buffer

**In opencl_utils.c:**
- `kernel_source_buffer[1MB]` - Kernel source code
- `build_log_buffer[16KB]` - Compilation error logs

### GPU Device Memory

Allocated dynamically via OpenCL APIs:
- Input buffer (image size)
- Output buffer (image size)
- Implicit local memory (for optimized kernels with `__local`)

---

## Testing and Validation

### Test Image Generation

**Script:** `scripts/generate_test_image.py`
- Creates 1024×1024 grayscale test image
- Includes geometric patterns and noise
- Output: `test_data/input.bin`

### Verification Strategies

**Dilate Algorithm:**
- Exact match required between GPU and reference
- Any pixel difference fails verification

**Gaussian Algorithm:**
- 1-pixel tolerance allowed
- Accounts for floating-point rounding differences
- GPU and reference may use different rounding strategies

### Performance Metrics

**Reported Metrics:**
- C reference execution time (milliseconds)
- GPU kernel execution time (from OpenCL profiling events)
- Speedup ratio (reference_time / gpu_time)
- Verification status (PASS/FAIL)

---

## Key Design Patterns

### 1. Plugin Architecture
Algorithms self-register at startup, enabling extensibility without modifying core code.

### 2. Static Polymorphism
Function pointers in `Algorithm` struct provide polymorphic behavior without dynamic dispatch overhead.

### 3. Configuration-Driven Execution
Kernel variants and parameters defined in INI file, allowing experimentation without recompilation.

### 4. Separation of Concerns
- Main: Orchestration
- Algorithms: Domain logic
- Utils: Infrastructure
- OpenCL: GPU abstraction

### 5. Reference Implementation Pattern
Each algorithm includes C reference for:
- Verification of GPU results
- Performance baseline
- Documentation of expected behavior

---

## File Reference

### Core Files

| File | Lines | Purpose |
|------|-------|---------|
| `src/main.c` | 317 | Entry point, menu system, orchestration |
| `src/utils/op_registry.c` | 63 | Algorithm registration and lookup |
| `src/utils/opencl_utils.c` | 180+ | OpenCL infrastructure and kernel management |
| `src/utils/config_parser.c` | 200+ | INI configuration parsing |
| `src/utils/image_io.c` | 90+ | Raw image reading and writing |
| `src/utils/safe_ops.h` | 92 | MISRA-C safe arithmetic operations |

### Algorithm Files

| File | Lines | Purpose |
|------|-------|---------|
| `src/dilate/dilate3x3.c` | 52 | Dilate algorithm wrapper |
| `src/dilate/c_ref/dilate3x3_ref.c` | 80 | Dilate C reference |
| `src/dilate/cl/dilate0.cl` | 23 | Basic dilate kernel |
| `src/dilate/cl/dilate1.cl` | 87 | Optimized dilate kernel |
| `src/gaussian/gaussian5x5.c` | 55 | Gaussian algorithm wrapper |
| `src/gaussian/c_ref/gaussian5x5_ref.c` | 92 | Gaussian C reference |
| `src/gaussian/cl/gaussian0.cl` | 33 | Gaussian kernel |

---

## Typical Execution Example

```
$ ./build/opencl_host

=== OpenCL Image Processing Framework ===

Available Operations:
1. Dilate 3x3
2. Gaussian 5x5
0. Exit
Select operation: 1

Available variants for 'dilate3x3':
1. v0 (dilate0.cl - dilate3x3)
2. v1 (dilate1.cl - dilate3x3_optimized)
Select variant: 2

Loading image: test_data/input.bin (1024x1024)
Running C reference implementation...
C Reference Time: 125.3 ms

Loading OpenCL kernel: src/dilate/cl/dilate1.cl
Compiling kernel: dilate3x3_optimized
Running OpenCL kernel...
OpenCL Kernel Time: 3.8 ms

Speedup: 32.97x
Verification: PASS

Saving output to: test_data/output.bin
Done.
```

---

## Summary

This OpenCL image processing framework demonstrates professional software engineering practices:

- **Modular Architecture:** Easy to extend with new algorithms
- **Safety-First:** 100% MISRA-C:2023 compliance with static allocation and overflow protection
- **Dual Implementation:** C reference for verification, OpenCL for performance
- **Configuration-Driven:** Runtime behavior controlled via INI files
- **Performance Focused:** Includes both basic and optimized kernel variants
- **Maintainable:** Clear separation of concerns, consistent interfaces, comprehensive error handling

The framework serves as an excellent foundation for learning GPU programming, experimenting with optimization techniques, and building production-quality image processing pipelines.
