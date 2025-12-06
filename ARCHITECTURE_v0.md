# OpenCL Codebase Architecture

Visual diagrams to understand the system architecture and execution flow.

---

## 1. High-Level System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                       CLI / USER INTERFACE                           │
│                          (src/main.c)                                │
│            • Argument parsing  • Menu system  • User I/O             │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     EXECUTION PIPELINE                               │
│                   (src/algorithm_runner.c)                           │
│     • Algorithm orchestration  • Buffer mgmt  • Verification         │
│                                                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  Algorithm   │  │   OpenCL     │  │    Cache     │              │
│  │   Registry   │  │    Utils     │  │   Manager    │              │
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
│           • CLI interface • Argument parsing • User menus            │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      EXECUTION PIPELINE LAYER                        │
│                    (src/algorithm_runner.c/.h)                       │
│    • Algorithm orchestration • Buffer management • Verification      │
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
│  │ op_registry.c/.h │  │opencl_utils.c/.h │  │  config.c/.h    │  │
│  │ • Algorithm reg  │  │ • Platform init  │  │ • INI parsing   │  │
│  │ • Registry mgmt  │  │ • Kernel exec    │  │ • Buffer config │  │
│  │                  │  │                  │  │ • Path utils    │  │
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
│      int (*set_kernel_args)(cl_kernel, cl_mem, cl_mem,            │
│                             const OpParams*);  /* Set args */     │
│  } Algorithm;                                                     │
│                                                                   │
│  OpParams includes:                                               │
│  - host_type: HOST_TYPE_STANDARD or HOST_TYPE_CL_EXTENSION       │
│  - kernel_variant: ID to distinguish different kernel signatures │
│  - custom_buffers: RuntimeBuffer array with metadata (type, size)│
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

# Kernel variants (supports multiple variants with different signatures)
# Note: kernel_variant is auto-derived from section name (v0->0, v1->1, v2->2)
[kernel.v0]
host_type = standard
kernel_file = src/gaussian/cl/gaussian0.cl
kernel_function = gaussian5x5
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

[kernel.v1]
host_type = cl_extension
kernel_file = src/gaussian/cl/gaussian1.cl
kernel_function = gaussian5x5_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

[kernel.v2]
host_type = cl_extension
kernel_file = src/gaussian/cl/gaussian2.cl
kernel_function = gaussian5x5_horizontal
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
│  [kernel.v0]                  ← Kernel variant 0 (auto)      │
│  host_type = standard                                        │
│  kernel_file = src/gaussian/cl/gaussian0.cl                  │
│  kernel_function = gaussian5x5                               │
│  global_work_size = 1920,1088                                │
│  local_work_size = 16,16                                     │
│                                                               │
│  [kernel.v1]                  ← Kernel variant 1 (auto)      │
│  host_type = cl_extension                                    │
│  kernel_file = src/gaussian/cl/gaussian1.cl                  │
│  kernel_function = gaussian5x5_optimized                     │
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
│     → KernelConfig[] (file, function, work sizes,            │
│                       host_type, kernel_variant)             │
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

main.c (Application Layer):
┌──────────────────────────────────────────────────────────────┐
│ static unsigned char gpu_output_buffer[16*1024*1024];       │  16 MB
│ static unsigned char ref_output_buffer[16*1024*1024];       │  16 MB
└──────────────────────────────────────────────────────────────┘

algorithm_runner.c (Execution Pipeline):
┌──────────────────────────────────────────────────────────────┐
│ RuntimeBuffer buffers[MAX_CUSTOM_BUFFERS];                   │  ~1 KB
│ CustomBuffers custom_buffers;                                │  (refs)
└──────────────────────────────────────────────────────────────┘

image_io.c (Infrastructure):
┌──────────────────────────────────────────────────────────────┐
│ static unsigned char image_buffer[16*1024*1024];            │  16 MB
└──────────────────────────────────────────────────────────────┘

opencl_utils.c (Infrastructure):
┌──────────────────────────────────────────────────────────────┐
│ static char kernel_source_buffer[1024*1024];                │   1 MB
│ static char build_log_buffer[16*1024];                      │  16 KB
└──────────────────────────────────────────────────────────────┘

config.c (Infrastructure):
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

## 10. Build System & Automation Tooling

```
                  ┌────────────────────────────────┐
                  │    scripts/build.sh            │
                  │  • Auto-generate registry      │
                  │  • Incremental compilation     │
                  │  • Clean option                │
                  └──────────────┬─────────────────┘
                                 │
                  ┌──────────────┴─────────────────┐
                  │  scripts/generate_registry.sh  │
                  │  • Scans for *_ref.c files     │
                  │  • Auto-generates registry     │
                  │  • Extracts display names      │
                  └──────────────┬─────────────────┘
                                 │
                                 ▼
                      ┌────────────────┐
                      │  src/Makefile  │
                      └────────┬───────┘
                               │
           ┌───────────────────┼───────────────────┐
           ▼                   ▼                   ▼
    ┌─────────────┐    ┌──────────────┐    ┌──────────┐
    │   main.c    │    │ algorithm_   │    │ utils/   │
    │  (CLI only) │    │  runner.c    │    │  *.c     │
    └─────────────┘    │ (pipeline)   │    └──────────┘
                       └──────────────┘         │
                              │           ┌─────┴──────┐
                              │           ▼            ▼
                              │    ┌──────────┐  ┌─────────────┐
                              │    │ dilate/  │  │ gaussian/   │
                              │    │  *.c     │  │   *.c       │
                              │    └──────────┘  └─────────────┘
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
                   │    ├── algorithm_      │
                   │    │     runner.o      │
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

AUTOMATION SCRIPTS (scripts/ directory):

BUILD & REGISTRY:
  • build.sh (2.3KB) - Main build script with auto-registry generation
  • generate_registry.sh (3.6KB) - Auto-discovers and registers algorithms
  • run.sh (543B) - Quick run wrapper

BENCHMARKING & TESTING:
  • run_all_variants.sh (6.6KB) - Comprehensive benchmarking tool
    - Runs all algorithm variants automatically
    - Real-time progress display with spinner
    - Performance rankings by GPU time
    - Colored output (PASSED/FAILED)
    - Summary report with speedup metrics
  • benchmark.sh (375B) - Build and benchmark all variants

METADATA EXTRACTION:
  • generate_kernel_json.py (6.1KB) - Extract kernel signatures to JSON
    - Parses .ini configuration files
    - Extracts OpenCL kernel function signatures
    - Generates structured JSON metadata
  • generate_all_kernel_json.sh (923B) - Batch JSON generation

ALGORITHM CREATION:
  • create_new_algo.sh (11KB) - Algorithm template generator
    - Creates directory structure
    - Generates C reference template
    - Generates OpenCL kernel template
    - Generates .ini configuration

TEST DATA GENERATION:
  • generate_test_image.py (1.2KB) - Synthetic test image generator
  • generate_gaussian_kernels.py (1.6KB) - Gaussian kernel weight generator

MODULAR STRUCTURE:
  • main.c (231 lines) - CLI only
  • algorithm_runner.c (413 lines) - Execution pipeline
  • utils/config.c - Extended with path utilities
  • Algorithm modules - Self-contained plugins

EXTERNAL DEPENDENCIES:
  • OpenCL Runtime (GPU drivers)
  • Standard C library (libc)
  • Math library (libm)
  • Python 3 (for scripts)
  • Bash (for build automation)

NO EXTERNAL LIBRARIES REQUIRED!
All algorithms self-contained.
```

---

## 11. Scripts & Automation Infrastructure

The `scripts/` directory contains comprehensive automation tooling for development, testing, and benchmarking.

### Build & Registry Automation

```
┌─────────────────────────────────────────────────────────────────┐
│                    BUILD WORKFLOW                               │
└─────────────────────────────────────────────────────────────────┘

1. scripts/build.sh
   │
   ├─► Invokes generate_registry.sh
   │   │
   │   ├─► Scans src/ for *_ref.c files
   │   │   - Finds: dilate3x3_ref.c, gaussian5x5_ref.c
   │   │
   │   ├─► Extracts algorithm IDs and display names
   │   │   - Reads config/*.ini files
   │   │   - Extracts [image] display_name fields
   │   │
   │   └─► Generates src/utils/auto_registry.c
   │       - Auto-generated extern declarations
   │       - Auto-populated algorithms[] array
   │       - Uses GCC constructor for pre-main init
   │
   ├─► Compiles source files
   │   - Incremental compilation (only changed files)
   │   - Generates build/obj/*.o
   │
   ├─► Links executable
   │   - Creates build/opencl_host
   │
   └─► Optional: Clean mode (./build.sh clean)
       - Removes build artifacts
       - Removes cache (golden/, kernels/)

OUTPUT: Ready-to-run executable with auto-discovered algorithms
```

### Benchmarking System

```
┌─────────────────────────────────────────────────────────────────┐
│            AUTOMATED BENCHMARKING WORKFLOW                      │
│               (scripts/run_all_variants.sh)                     │
└─────────────────────────────────────────────────────────────────┘

For each algorithm (dilate3x3, gaussian5x5):
  For each variant (v0, v1, v2, ...):
    │
    ├─► Execute: build/opencl_host <algo_id> <variant>
    │
    ├─► Capture output and parse:
    │   - Verification status (PASSED/FAILED)
    │   - CPU time (ms)
    │   - GPU time (ms)
    │   - Speedup (Nx)
    │
    ├─► Display real-time progress:
    │   [Running dilate3x3 v0... ⠋]
    │   ✓ dilate3x3 v0: PASSED (GPU: 0.003ms, Speedup: 817x)
    │
    └─► Handle errors gracefully:
        ✗ gaussian5x5 v2: FAILED (verification error)

Final Report:
┌─────────────────────────────────────────────────────────────────┐
│ Performance Ranking (by GPU time):                             │
│ 1. dilate3x3 v1:    0.002ms  (Speedup: 1250x)  ✓               │
│ 2. dilate3x3 v0:    0.003ms  (Speedup: 817x)   ✓               │
│ 3. gaussian5x5 v0:  0.012ms  (Speedup: 208x)   ✓               │
│ 4. gaussian5x5 v1:  0.015ms  (Speedup: 167x)   ✓               │
│ 5. gaussian5x5 v2:  0.008ms  (Speedup: 312x)   ✗ FAILED        │
└─────────────────────────────────────────────────────────────────┘

FEATURES:
  ✓ Real-time progress with spinner animation
  ✓ Colored output (green=PASSED, red=FAILED)
  ✓ Performance ranking by GPU execution time
  ✓ Automatic failure detection and reporting
  ✓ Summary statistics across all variants
```

### Metadata Extraction System

```
┌─────────────────────────────────────────────────────────────────┐
│            KERNEL METADATA EXTRACTION                           │
│          (scripts/generate_kernel_json.py)                      │
└─────────────────────────────────────────────────────────────────┘

INPUT: config/gaussian5x5.ini

PROCESSING:
1. Parse INI file
   ├─► Extract kernel variant sections ([kernel.v0], [kernel.v1], ...)
   └─► Extract kernel_file paths

2. For each kernel variant:
   ├─► Read OpenCL kernel source (.cl file)
   ├─► Parse __kernel function signature
   │   - Extract function name
   │   - Extract parameter types and names
   │   - Handle __global, __local, __constant qualifiers
   └─► Build structured metadata

OUTPUT: JSON metadata file
{
  "algorithm": "gaussian5x5",
  "variants": [
    {
      "variant_id": 0,
      "kernel_file": "src/gaussian/cl/gaussian0.cl",
      "function_name": "gaussian5x5",
      "host_type": "standard",
      "parameters": [
        {"type": "__global uchar*", "name": "input"},
        {"type": "__global uchar*", "name": "output"},
        {"type": "int", "name": "width"},
        {"type": "int", "name": "height"},
        {"type": "__global float*", "name": "kernel_x"},
        {"type": "__global float*", "name": "kernel_y"}
      ]
    },
    {
      "variant_id": 1,
      "kernel_file": "src/gaussian/cl/gaussian1.cl",
      "function_name": "gaussian5x5_optimized",
      "host_type": "cl_extension",
      "parameters": [
        {"type": "__global uchar*", "name": "input"},
        {"type": "__global uchar*", "name": "tmp_global"},
        {"type": "__global uchar*", "name": "output"},
        {"type": "int", "name": "width"},
        {"type": "int", "name": "height"},
        {"type": "__global float*", "name": "kernel_x"},
        {"type": "__global float*", "name": "kernel_y"}
      ]
    }
  ]
}

USE CASES:
  • Documentation generation
  • IDE code completion
  • Automated testing
  • API documentation
  • Code analysis
```

### Algorithm Creation Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│           NEW ALGORITHM CREATION                                │
│          (scripts/create_new_algo.sh)                           │
└─────────────────────────────────────────────────────────────────┘

Usage: ./create_new_algo.sh <algorithm_name>
Example: ./create_new_algo.sh sobel3x3

PROCESS:
1. Validate algorithm name
   - Check format (lowercase, alphanumeric + underscore)
   - Check for duplicates

2. Create directory structure:
   src/<algo_name>/
   ├── <algo_name>.h          ← Public interface header
   ├── <algo_name>.c          ← Main implementation with callbacks
   ├── c_ref/
   │   ├── <algo_name>_ref.h  ← C reference header
   │   └── <algo_name>_ref.c  ← C reference implementation
   └── cl/
       └── <algo_name>0.cl    ← OpenCL kernel template

3. Generate template files:
   ├─► <algo_name>.c contains:
   │   - reference_impl() function (calls C reference)
   │   - verify_result() function (tolerance-based comparison)
   │   - set_kernel_args() function (sets CL kernel arguments)
   │   - Algorithm struct registration
   │
   ├─► c_ref/*.c contains:
   │   - CPU reference implementation skeleton
   │   - TODO comments for implementation
   │
   └─► cl/*.cl contains:
       - __kernel function skeleton
       - Work-item ID calculations
       - TODO comments for implementation

4. Generate configuration file:
   config/<algo_name>.ini
   ├─► [image] section (I/O configuration)
   ├─► [buffer.*] sections (custom buffers, if needed)
   └─► [kernel.v0] section (kernel variant configuration)

5. Instructions displayed:
   ✓ Algorithm structure created in src/<algo_name>/
   ✓ Configuration created in config/<algo_name>.ini

   NEXT STEPS:
   1. Implement C reference in src/<algo_name>/c_ref/
   2. Implement OpenCL kernel in src/<algo_name>/cl/
   3. Update set_kernel_args() for your buffer layout
   4. Run: ./scripts/build.sh && ./scripts/run.sh
```

### Test Data Generation

```
┌─────────────────────────────────────────────────────────────────┐
│              TEST DATA GENERATION                               │
└─────────────────────────────────────────────────────────────────┘

scripts/generate_test_image.py
├─► Creates synthetic test images
├─► Supports various patterns:
│   - Checkerboard
│   - Gradients
│   - Geometric shapes
└─► Output: Binary .raw format (unsigned char)

scripts/generate_gaussian_kernels.py
├─► Generates Gaussian kernel weights
├─► Configurable sigma parameter
├─► Creates separable 1D kernels
└─► Output: kernel_x.bin, kernel_y.bin (float32)

Usage Examples:
  python3 scripts/generate_test_image.py 1024 1024 checkerboard
  python3 scripts/generate_gaussian_kernels.py --sigma 1.4 --size 5
```

### Quick Reference Commands

```bash
# Build the project (auto-generates registry)
./scripts/build.sh

# Clean build (removes artifacts and cache)
./scripts/build.sh clean

# Run executable
./scripts/run.sh

# Benchmark all algorithm variants
./scripts/benchmark.sh

# Run specific algorithm variant
build/opencl_host dilate3x3 0    # Run dilate3x3 variant 0
build/opencl_host gaussian5x5 1  # Run gaussian5x5 variant 1

# Create new algorithm
./scripts/create_new_algo.sh my_algorithm

# Extract kernel metadata to JSON
python3 scripts/generate_kernel_json.py config/gaussian5x5.ini

# Generate all JSON metadata
./scripts/generate_all_kernel_json.sh
```

---

## Summary

This architecture provides:

1. **Modularity** - Self-contained algorithm plugins with clean interfaces
   - CLI layer (main.c) - 231 lines
   - Execution pipeline (algorithm_runner.c) - 413 lines
   - Infrastructure utilities (utils/) - Reusable components
   - 2 complete algorithms with 5 kernel variants

2. **Extensibility** - Add algorithms without modifying core code
   - Auto-discovery system scans for new algorithms
   - Template generator (create_new_algo.sh) for rapid development
   - No manual registration required

3. **Flexibility** - Per-algorithm INI files with custom buffer configuration
   - Support for file-backed and empty buffers
   - Multiple kernel variants per algorithm
   - Auto-derived variant IDs from section names

4. **Performance** - Binary caching accelerates iteration (50-60x)
   - Kernel binary caching (~100x faster compilation)
   - Golden sample caching (~50x faster CPU reference)
   - Automated benchmarking for performance tracking

5. **Safety** - MISRA-C compliant (static allocation, safe arithmetic)
   - No dynamic allocation in hot paths
   - Bounds checking and safe operations
   - Deterministic memory layout

6. **Verification** - CPU reference validates GPU correctness
   - Algorithm-specific tolerance levels
   - Automatic verification on every run
   - Detailed error reporting

7. **Scalability** - Multiple algorithms, multiple variants per algorithm
   - Clean variant management system
   - Support for different optimization strategies
   - API selection (standard vs. cl_extension)

8. **Automation** - Comprehensive tooling infrastructure
   - **Build automation**: Auto-registry generation and incremental compilation
   - **Benchmarking**: Automated performance testing across all variants
   - **Metadata extraction**: JSON export for documentation and analysis
   - **Algorithm creation**: Template generator for new algorithms
   - **Test data generation**: Synthetic image and kernel weight generation

9. **Maintainability** - Clear separation of concerns:
   - Application layer: CLI interface
   - Execution layer: Algorithm orchestration
   - Algorithm layer: Self-contained plugins
   - Infrastructure layer: Reusable utilities
   - Automation layer: Development and testing scripts

The design follows professional software engineering practices suitable for client demonstrations, research, and production deployment. The comprehensive automation infrastructure enables rapid algorithm development, testing, and performance optimization.
