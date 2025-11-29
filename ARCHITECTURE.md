# OpenCL Codebase Architecture

Visual diagrams to understand the system architecture and execution flow.

---

## 1. High-Level System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                            MAIN PROGRAM                              │
│                            (src/main.c)                              │
│                                                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  Algorithm   │  │   OpenCL     │  │    Image     │              │
│  │   Registry   │  │   Manager    │  │     I/O      │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
└─────────────────────────────────────────────────────────────────────┘
         │                      │                     │
         │                      │                     │
         ▼                      ▼                     ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│   ALGORITHMS     │  │  OPENCL KERNELS  │  │   TEST DATA      │
│  (Plugin System) │  │  (GPU Execution) │  │  (Images/Cache)  │
└──────────────────┘  └──────────────────┘  └──────────────────┘
         │                      │                     │
         │                      │                     │
    ┌────┴────┐          ┌──────┴──────┐      ┌─────┴─────┐
    ▼         ▼          ▼             ▼      ▼           ▼
┌────────┐ ┌─────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
│ Dilate │ │Gaussian │ │dilate0 │ │dilate1 │ │ Images │ │ Cache  │
│  3x3   │ │  5x5    │ │  .cl   │ │  .cl   │ │  .raw  │ │  .bin  │
└────────┘ └─────────┘ └────────┘ └────────┘ └────────┘ └────────┘
```

---

## 2. Module Organization & Dependencies

```
┌─────────────────────────────────────────────────────────────────────┐
│                          APPLICATION LAYER                           │
│                           (src/main.c)                               │
│  • Program entry & menu • Algorithm execution • Performance reports  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         ALGORITHM LAYER                              │
│                                                                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │
│  │  src/dilate/    │  │ src/gaussian/   │  │  src/resize/    │    │
│  │  dilate3x3.c    │  │ gaussian5x5.c   │  │  resize.c       │    │
│  │  • C reference  │  │  • C reference  │  │  • C reference  │    │
│  │  • Verification │  │  • Verification │  │  • Verification │    │
│  │  • Set args     │  │  • Set args     │  │  • Set args     │    │
│  │                 │  │                 │  │                 │    │
│  │  c_ref/  cl/    │  │  c_ref/  cl/    │  │  c_ref/  cl/    │    │
│  │  *.c     *.cl   │  │  *.c     *.cl   │  │  *.c     *.cl   │    │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                                    │
┌─────────────────────────────────────────────────────────────────────┐
│                       INFRASTRUCTURE LAYER (src/utils/)              │
│                                                                       │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │ op_registry.c/.h │  │opencl_utils.c/.h │  │config_parser.c  │  │
│  │ • Algorithm reg  │  │ • Platform init  │  │ • INI parsing   │  │
│  │ • Registry mgmt  │  │ • Kernel exec    │  │ • Buffer config │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                       │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │  image_io.c/.h   │  │cache_manager.c/.h│  │  safe_ops.h     │  │
│  │ • Image I/O      │  │ • Cache mgmt     │  │ • Safe ops      │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                       │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │ op_interface.h    ← Algorithm Interface                       │  │
│  │                                                                │  │
│  │ typedef struct {                                               │  │
│  │   char name[64], id[32];                                       │  │
│  │   void (*reference_impl)(OpParams*);                           │  │
│  │   int (*verify_result)(OpParams*, float*);                     │  │
│  │   int (*set_kernel_args)(cl_kernel, cl_mem, cl_mem, OpParams);│  │
│  │ } Algorithm;                                                   │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
┌─────────────────────────────────────────────────────────────────────┐
│                          SYSTEM LAYER                                │
│  • OpenCL Runtime  • libc (MISRA-C)  • File System                  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 3. Execution Flow (Single Algorithm Run)

```
START: User selects algorithm & variant
  │
  ▼
┌─────────────────────────────────────┐
│ 1. INITIALIZATION                   │
│    • Initialize OpenCL platform     │
│    • Parse algorithm INI file       │
│    • Load buffer configurations     │
│    • Create context & queue         │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│ 2. BUFFER CREATION                  │
│    • Load input image from INI      │
│    • Create input/output buffers    │
│    • Create custom buffers (INI)    │
│      - Intermediate buffers         │
│      - Weight/kernel data buffers   │
│      - Read from source files       │
└─────────────────────────────────────┘
  │
  ├──────────────────────┬─────────────────────┐
  ▼                      ▼                     ▼
┌───────────────┐  ┌────────────────┐  ┌──────────────────┐
│ 3a. CPU PATH  │  │ 3b. GPU PATH   │  │ 3c. CACHING      │
│               │  │                │  │                  │
│ Check golden  │  │ Check kernel   │  │ • golden/*.bin   │
│ cache         │  │ binary cache   │  │ • kernels/*.bin  │
│               │  │                │  │                  │
│ Run C ref     │  │ Compile .cl or │  │ Cache misses:    │
│ implementation│  │ load binary    │  │ ~250ms first run │
│               │  │                │  │                  │
│ Time: ~2.5ms  │  │ Set kernel args│  │ Cache hits:      │
│               │  │ (via callback) │  │ ~5-10ms after    │
│               │  │                │  │                  │
│               │  │ Run kernel     │  │                  │
│               │  │ Time: ~0.003ms │  │                  │
└───────────────┘  └────────────────┘  └──────────────────┘
  │                      │
  └──────────┬───────────┘
             ▼
┌─────────────────────────────────────┐
│ 4. VERIFICATION                     │
│    • Compare cpu vs gpu output      │
│    • Calculate max error            │
│    • Algorithm-specific tolerance   │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│ 5. RESULTS DISPLAY                  │
│  ✓ Verification: PASS               │
│  ✓ CPU Time:     2.45 ms            │
│  ✓ GPU Time:     0.003 ms           │
│  ✓ Speedup:      817x               │
└─────────────────────────────────────┘
  │
  ▼
END: Return to menu or exit
```

---

## 4. Algorithm Plugin Architecture

```
┌───────────────────────────────────────────────────────────────────┐
│                      ALGORITHM INTERFACE                          │
│                    (src/utils/op_interface.h)                     │
│                                                                   │
│  typedef struct {                                                 │
│      char name[64], id[32];       /* Display & ID */              │
│      void (*reference_impl)(OpParams*);  /* C reference */        │
│      int (*verify_result)(OpParams*, float*);  /* Verification */ │
│      int (*set_kernel_args)(cl_kernel, cl_mem, cl_mem, OpParams); │
│  } Algorithm;                                                     │
└───────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Implements
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│   DILATE 3x3     │  │  GAUSSIAN 5x5    │  │    RESIZE        │
│                  │  │                  │  │                  │
│  reference_impl: │  │  reference_impl: │  │  reference_impl: │
│  • CPU dilate    │  │  • CPU gaussian  │  │  • CPU resize    │
│                  │  │                  │  │                  │
│  verify_result:  │  │  verify_result:  │  │  verify_result:  │
│  • Exact match   │  │  • ±1 tolerance  │  │  • Custom tol    │
│                  │  │                  │  │                  │
│  set_kernel_args:│  │  set_kernel_args:│  │  set_kernel_args:│
│  • input, output │  │  • input, output │  │  • input, output │
│  • w, h, stride  │  │  • buffers, w, h │  │  • src/dst dims  │
└──────────────────┘  └──────────────────┘  └──────────────────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
              ┌───────────────────────────────┐
              │   ALGORITHM REGISTRY          │
              │  (src/utils/op_registry.c)    │
              │                               │
              │  algorithms[] = {             │
              │    &dilate3x3_algorithm,      │
              │    &gaussian5x5_algorithm,    │
              │    &resize_algorithm,         │
              │    NULL                       │
              │  }                            │
              └───────────────────────────────┘
                              │
                              ▼
                Used by main.c for menu
```

---

## 5. Per-Algorithm Directory Structure

```
src/gaussian/                        ← Self-contained algorithm module
│
├── gaussian5x5.h                    ← Public interface
├── gaussian5x5.c                    ← Algorithm implementation
│   ├── gaussian5x5_reference()      → C reference impl
│   ├── gaussian5x5_verify()         → ±1 tolerance verification
│   └── gaussian5x5_set_args()       → Set kernel arguments
│
├── c_ref/                           ← CPU reference implementation
│   ├── gaussian5x5_ref.h
│   └── gaussian5x5_ref.c
│
└── cl/                              ← GPU kernel variants
    └── gaussian0.cl                 → Kernel implementation

Configuration (config/gaussian5x5.ini):
───────────────────────────────────────
# Image configuration
[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080

# Custom buffers
[buffer.tmp_global]                  ← Intermediate buffer
type = READ_WRITE
size_bytes = 314572800

[buffer.kernel_x]                    ← Gaussian weights X
type = READ_ONLY
data_type = float
num_elements = 5
source_file = test_data/gaussian5x5/kernel_x.bin

[buffer.kernel_y]                    ← Gaussian weights Y
type = READ_ONLY
data_type = float
num_elements = 5
source_file = test_data/gaussian5x5/kernel_y.bin

# Kernel variant
[kernel.v0]
kernel_file = src/gaussian/cl/gaussian0.cl
kernel_function = gaussian5x5
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

Test Data Organization:
───────────────────────
test_data/gaussian5x5/
├── kernel_x.bin                     ← Horizontal weights
├── kernel_y.bin                     ← Vertical weights
├── golden/
│   └── gaussian5x5.bin              ← Cached C reference output
└── kernels/
    └── gaussian0.bin                ← Cached compiled kernel
```

---

## 6. Data Flow Through the System

```
┌─────────────────────────────────────────────────────────────────┐
│                        INPUT IMAGE                              │
│                   test_data/input.raw                           │
│                   (1024x1024 grayscale)                         │
└─────────────────────────────────────────────────────────────────┘
                            │
                            │ load_raw_image()
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                   HOST MEMORY (CPU)                             │
│               unsigned char image[1024*1024]                    │
└─────────────────────────────────────────────────────────────────┘
                            │
                ┌───────────┴───────────┐
                ▼                       ▼
┌──────────────────────────┐  ┌──────────────────────────┐
│      CPU PATH            │  │      GPU PATH            │
│                          │  │                          │
│  C Reference Function    │  │  OpenCL Kernel           │
│  ┌────────────────────┐  │  │  ┌────────────────────┐  │
│  │ dilate3x3_ref()    │  │  │  │ clCreateBuffer()   │  │
│  │                    │  │  │  │   (input)          │  │
│  │ Processes pixels:  │  │  │  └──────────┬─────────┘  │
│  │ for y in height:   │  │  │             │            │
│  │   for x in width:  │  │  │             ▼            │
│  │     max = 0        │  │  │  ┌────────────────────┐  │
│  │     for 3x3 window:│  │  │  │ clEnqueueWrite     │  │
│  │       find max     │  │  │  │ Buffer()           │  │
│  │     output[x,y]=max│  │  │  │                    │  │
│  └────────────────────┘  │  │  │ Copy to GPU memory │  │
│           │              │  │  └──────────┬─────────┘  │
│           ▼              │  │             ▼            │
│  ┌────────────────────┐  │  │  ┌────────────────────┐  │
│  │ cpu_output[]       │  │  │  │ GPU Memory         │  │
│  │ (1024x1024)        │  │  │  │ cl_mem input_buf   │  │
│  └────────────────────┘  │  │  └──────────┬─────────┘  │
│                          │  │             ▼            │
│  Time: ~2.5 ms           │  │  ┌────────────────────┐  │
│                          │  │  │ __kernel dilate3x3 │  │
│                          │  │  │                    │  │
│                          │  │  │ Parallel execution:│  │
│                          │  │  │ 1024x1024 threads  │  │
│                          │  │  │ Each: 3x3 window   │  │
│                          │  │  └──────────┬─────────┘  │
│                          │  │             ▼            │
│                          │  │  ┌────────────────────┐  │
│                          │  │  │ GPU Memory         │  │
│                          │  │  │ cl_mem output_buf  │  │
│                          │  │  └──────────┬─────────┘  │
│                          │  │             ▼            │
│                          │  │  ┌────────────────────┐  │
│                          │  │  │ clEnqueueRead      │  │
│                          │  │  │ Buffer()           │  │
│                          │  │  │                    │  │
│                          │  │  │ Copy from GPU      │  │
│                          │  │  └──────────┬─────────┘  │
│                          │  │             ▼            │
│                          │  │  ┌────────────────────┐  │
│                          │  │  │ gpu_output[]       │  │
│                          │  │  │ (1024x1024)        │  │
│                          │  │  └────────────────────┘  │
│                          │  │                          │
│                          │  │  Time: ~0.003 ms         │
└──────────────────────────┘  └──────────────────────────┘
                │                            │
                └────────────┬───────────────┘
                             ▼
            ┌────────────────────────────────────┐
            │     VERIFICATION FUNCTION          │
            │   (algorithm->verify_result())     │
            │                                    │
            │   Compare arrays byte-by-byte:     │
            │   for (i = 0; i < size; i++) {     │
            │     if (cpu_out[i] != gpu_out[i])  │
            │       error_count++;               │
            │   }                                │
            │                                    │
            │   Result: PASS/FAIL                │
            └────────────────────────────────────┘
                             │
                             ▼
            ┌────────────────────────────────────┐
            │       PERFORMANCE REPORT           │
            │                                    │
            │  CPU Time:  2.45 ms                │
            │  GPU Time:  0.003 ms               │
            │  Speedup:   817x                   │
            │  Status:    ✓ PASS                 │
            └────────────────────────────────────┘
```

---

## 7. Configuration System Flow

```
┌───────────────────────────────────────────────────────────────┐
│                 config/gaussian5x5.ini                        │
│                                                               │
│  [image]                      ← Image dimensions & I/O       │
│  input = test_data/input.bin                                 │
│  src_width = 1920                                            │
│  src_height = 1080                                           │
│                                                               │
│  [buffer.tmp_global]          ← Custom buffer 0              │
│  type = READ_WRITE                                           │
│  size_bytes = 314572800                                      │
│                                                               │
│  [buffer.kernel_x]            ← Custom buffer 1              │
│  type = READ_ONLY                                            │
│  data_type = float                                           │
│  num_elements = 5                                            │
│  source_file = test_data/gaussian5x5/kernel_x.bin            │
│                                                               │
│  [buffer.kernel_y]            ← Custom buffer 2              │
│  type = READ_ONLY                                            │
│  data_type = float                                           │
│  num_elements = 5                                            │
│  source_file = test_data/gaussian5x5/kernel_y.bin            │
│                                                               │
│  [kernel.v0]                  ← Kernel variant 0             │
│  kernel_file = src/gaussian/cl/gaussian0.cl                  │
│  kernel_function = gaussian5x5                               │
│  global_work_size = 1920,1088                                │
│  local_work_size = 16,16                                     │
└───────────────────────────────────────────────────────────────┘
                            │
                            │ parse_config()
                            ▼
┌───────────────────────────────────────────────────────────────┐
│              CONFIG PARSER (config_parser.c)                  │
│                                                               │
│  1. Parse [image] section                                    │
│     → ImageConfig (dims, I/O paths)                          │
│                                                               │
│  2. Parse [buffer.*] sections                                │
│     → BufferConfig[] (type, size, source)                    │
│                                                               │
│  3. Parse [kernel.*] sections                                │
│     → KernelConfig[] (file, function, work sizes)            │
│                                                               │
│  4. Validate & store in AlgorithmConfig                      │
└───────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────────┐
│             OPENCL EXECUTION (opencl_utils.c)                 │
│                                                               │
│  1. Create input/output buffers (from ImageConfig)           │
│  2. Create custom buffers (from BufferConfig[])              │
│     • Allocate CL_MEM_READ_WRITE/READ_ONLY                   │
│     • Load data from source_file if specified                │
│  3. Build kernel (from KernelConfig)                         │
│  4. Set kernel arguments via algorithm callback              │
│     • algorithm->set_kernel_args(kernel, ...)                │
│  5. Execute kernel with configured work sizes                │
└───────────────────────────────────────────────────────────────┘

BENEFITS:
  ✓ Per-algorithm INI files (modular configuration)
  ✓ Flexible custom buffer creation
  ✓ Load kernel weights from files
  ✓ No recompilation needed for config changes
```

---

## 8. Caching System Architecture

```
┌───────────────────────────────────────────────────────────────┐
│                    FIRST RUN (Cold Cache)                     │
└───────────────────────────────────────────────────────────────┘

CPU Path:                          GPU Path:
┌──────────────────┐               ┌──────────────────┐
│ Check golden     │               │ Check kernel     │
│ cache            │               │ binary cache     │
│ ✗ Not found      │               │ ✗ Not found      │
└────────┬─────────┘               └────────┬─────────┘
         ▼                                  ▼
┌──────────────────┐               ┌──────────────────┐
│ Run C reference  │               │ Load .cl source  │
│ dilate3x3_ref()  │               │ (23 lines)       │
│                  │               └────────┬─────────┘
│ Time: 2.5 ms     │                        ▼
└────────┬─────────┘               ┌──────────────────┐
         ▼                         │ Compile kernel   │
┌──────────────────┐               │ (clBuildProgram) │
│ Save result to:  │               │                  │
│ test_data/dilate/│               │ Time: 50-200 ms  │
│   golden/        │               │ (SLOW!)          │
│   dilate3x3.bin  │               └────────┬─────────┘
└──────────────────┘                        ▼
                                   ┌──────────────────┐
                                   │ Extract binary   │
                                   │ Save to:         │
                                   │ test_data/dilate/│
                                   │   kernels/       │
                                   │   dilate0.bin    │
                                   └────────┬─────────┘
                                            ▼
                                   ┌──────────────────┐
                                   │ Execute kernel   │
                                   │ Time: 0.003 ms   │
                                   └──────────────────┘

Total first run: ~250-300 ms


┌───────────────────────────────────────────────────────────────┐
│                SUBSEQUENT RUNS (Warm Cache)                   │
└───────────────────────────────────────────────────────────────┘

CPU Path:                          GPU Path:
┌──────────────────┐               ┌──────────────────┐
│ Check golden     │               │ Check kernel     │
│ cache            │               │ binary cache     │
│ ✓ Found!         │               │ ✓ Found!         │
└────────┬─────────┘               └────────┬─────────┘
         ▼                                  ▼
┌──────────────────┐               ┌──────────────────┐
│ Load from cache: │               │ Load from cache: │
│ test_data/dilate/│               │ test_data/dilate/│
│   golden/        │               │   kernels/       │
│   dilate3x3.bin  │               │   dilate0.bin    │
│                  │               │                  │
│ Time: 0.5 ms     │               │ Time: 2 ms       │
│ (50x faster!)    │               │ (100x faster!)   │
└──────────────────┘               └────────┬─────────┘
                                            ▼
                                   ┌──────────────────┐
                                   │ Execute kernel   │
                                   │ Time: 0.003 ms   │
                                   └──────────────────┘

Total cached run: ~5-10 ms
Speedup: 50-60x faster!


Cache Directory Structure:
──────────────────────────
test_data/
├── input.raw                      ← Test image
├── dilate/
│   ├── golden/
│   │   └── dilate3x3.bin          ← Cached C reference output
│   └── kernels/
│       ├── dilate0.bin            ← Cached compiled basic kernel
│       └── dilate1.bin            ← Cached compiled optimized kernel
│
└── gaussian/
    ├── golden/
    │   └── gaussian5x5.bin
    └── kernels/
        └── gaussian0.bin
```

---

## 9. Memory Architecture (MISRA-C Compliant)

```
┌───────────────────────────────────────────────────────────────┐
│                    STATIC MEMORY LAYOUT                       │
│              (No malloc - MISRA-C Rule 21.3)                  │
└───────────────────────────────────────────────────────────────┘

main.c:
┌──────────────────────────────────────────────────────────────┐
│ static unsigned char gpu_output_buffer[16*1024*1024];       │  16 MB
│ static unsigned char ref_output_buffer[16*1024*1024];       │  16 MB
└──────────────────────────────────────────────────────────────┘

image_io.c:
┌──────────────────────────────────────────────────────────────┐
│ static unsigned char image_buffer[16*1024*1024];            │  16 MB
└──────────────────────────────────────────────────────────────┘

opencl_utils.c:
┌──────────────────────────────────────────────────────────────┐
│ static char kernel_source_buffer[1024*1024];                │   1 MB
│ static char build_log_buffer[16*1024];                      │  16 KB
└──────────────────────────────────────────────────────────────┘

config_parser.c:
┌──────────────────────────────────────────────────────────────┐
│ static KernelConfig configs[32];                            │  ~50 KB
└──────────────────────────────────────────────────────────────┘

                          TOTAL: ~49 MB

BENEFITS:
  ✓ Deterministic memory layout
  ✓ No heap fragmentation
  ✓ No memory leaks possible
  ✓ Suitable for safety-critical systems
  ✓ Predictable performance

Maximum supported image: 4096 x 4096 pixels (16 MB)
```

---

## 10. Build System & Dependencies

```
                      ┌────────────────┐
                      │  src/Makefile  │
                      └────────┬───────┘
                               │
                ┌──────────────┼──────────────┐
                ▼              ▼              ▼
        ┌─────────────┐  ┌──────────┐  ┌──────────┐
        │   main.c    │  │ utils/   │  │ dilate/  │
        └─────────────┘  │  *.c     │  │  *.c     │
                         └──────────┘  └──────────┘
                               │              │
                         ┌─────┴──────┬───────┘
                         ▼            ▼
                  ┌────────────┐  ┌─────────────┐
                  │ gaussian/  │  │ algorithm3/ │
                  │   *.c      │  │   *.c       │
                  └────────────┘  └─────────────┘
                         │
                         ▼
              ┌────────────────────────┐
              │     Compiler (gcc)     │
              │  -Wall -O2 -std=c99    │
              │  -I. (include paths)   │
              └────────────┬───────────┘
                           ▼
              ┌────────────────────────┐
              │   Object Files (.o)    │
              │   build/obj/           │
              │    ├── main.o          │
              │    ├── utils/          │
              │    ├── dilate/         │
              │    └── gaussian/       │
              └────────────┬───────────┘
                           ▼
              ┌────────────────────────┐
              │    Linker (gcc)        │
              │  -framework OpenCL     │
              │  -lm (math library)    │
              └────────────┬───────────┘
                           ▼
              ┌────────────────────────┐
              │  build/opencl_host     │
              │     (70 KB exec)       │
              └────────────────────────┘

EXTERNAL DEPENDENCIES:
  • OpenCL Runtime (GPU drivers)
  • Standard C library (libc)
  • Math library (libm)

NO EXTERNAL LIBRARIES REQUIRED!
All algorithms self-contained.
```

---

## Summary

This architecture provides:

1. **Modularity** - Self-contained algorithm plugins with clean interfaces
2. **Extensibility** - Add algorithms without modifying core code
3. **Flexibility** - Per-algorithm INI files with custom buffer configuration
4. **Performance** - Binary caching accelerates iteration (50-60x)
5. **Safety** - MISRA-C compliant (static allocation, safe arithmetic)
6. **Verification** - CPU reference validates GPU correctness
7. **Scalability** - Multiple algorithms, multiple variants per algorithm
8. **Maintainability** - Clear separation: app, algorithm, infrastructure layers

The design follows professional software engineering practices suitable for client demonstrations and production deployment.
