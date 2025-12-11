# OpenCL Codebase Architecture

Visual diagrams to understand the SDK-ready system architecture and execution flow.

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
│                   LIBRARY: libopencl_imgproc                         │
│                 (Core + Platform + Utils)                            │
│ ┌─────────────────────────────────────────────────────────────────┐ │
│ │              EXECUTION PIPELINE (core/)                         │ │
│ │            (src/core/algorithm_runner.c)                        │ │
│ │   • Algorithm orchestration  • Buffer mgmt  • Verification      │ │
│ │                                                                 │ │
│ │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │ │
│ │  │  Algorithm   │  │   OpenCL     │  │    Cache     │         │ │
│ │  │   Registry   │  │    Utils     │  │   Manager    │         │ │
│ │  │   (core/)    │  │  (platform/) │  │  (platform/) │         │ │
│ │  └──────────────┘  └──────────────┘  └──────────────┘         │ │
│ └─────────────────────────────────────────────────────────────────┘ │
│                                                                       │
│ ┌─────────────────────────────────────────────────────────────────┐ │
│ │                  UTILITIES (utils/)                             │ │
│ │  • Config parser  • Image I/O  • Verification  • Safe ops      │ │
│ └─────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
         │                      │                     │
         │                      │                     │
         ▼                      ▼                     ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│   ALGORITHMS     │  │  OPENCL KERNELS  │  │   TEST DATA      │
│ (examples/ dir)  │  │  (GPU Execution) │  │  (Images/Cache)  │
│  User Plugins    │  │                  │  │                  │
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

**Key Design Principles**:
- Library separation: Core + Platform + Utils packaged as `libopencl_imgproc`
- Algorithms in `examples/` (user code, not part of library)
- Clean separation for SDK distribution

---

## 2. Module Organization & Dependencies (Clean Architecture)

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
│                      USER ALGORITHM LAYER                            │
│                        (examples/ directory)                         │
│  ┌─────────────────┐  ┌─────────────────┐                          │
│  │ examples/dilate/│  │examples/gaussian│                          │
│  │  c_ref/         │  │  c_ref/         │                          │
│  │  dilate3x3_ref.c│  │  gaussian5x5_   │                          │
│  │                 │  │    ref.c        │                          │
│  │ • C reference   │  │ • C reference   │                          │
│  │   impl ONLY     │  │   impl ONLY     │                          │
│  │                 │  │                 │                          │
│  │  Depends ONLY   │  │  Depends ONLY   │                          │
│  │  on include/    │  │  on include/    │                          │
│  └─────────────────┘  └─────────────────┘                          │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    PUBLIC API LAYER (Stable Interface)               │
│                         (include/ directory)                         │
│                                                                       │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │ include/op_interface.h    ← Algorithm Interface               │  │
│  │ typedef struct {                                               │  │
│  │   char name[64], id[32];                                       │  │
│  │   void (*reference_impl)(OpParams*);   // C reference only!   │  │
│  │ } Algorithm;                                                   │  │
│  │                                                                │  │
│  │ NOTE: kernel_args and verification are config-driven via INI  │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                       │
│  ┌──────────────────┐  ┌──────────────────┐                        │
│  │ op_registry.h    │  │algorithm_runner.h│                        │
│  │ • Registration   │  │ • Forward decls  │                        │
│  │   macros         │  │   only           │                        │
│  └──────────────────┘  └──────────────────┘                        │
│                                                                       │
│  ┌──────────────────────────────────────────┐                      │
│  │ include/utils/ (Public Utilities)         │                      │
│  │  • safe_ops.h    - Safe arithmetic        │                      │
│  │  • verify.h      - Verification functions │                      │
│  └──────────────────────────────────────────┘                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
┌─────────────────────────────────────────────────────────────────────┐
│        LIBRARY: libopencl_imgproc.so/.a (Internal Implementation)   │
│                                                                       │
│ ┌─────────────────────────────────────────────────────────────────┐ │
│ │                  CORE LAYER (Business Logic)                    │ │
│ │                      (src/core/)                                │ │
│ │                                                                 │ │
│ │  ┌──────────────────┐  ┌──────────────────┐                   │ │
│ │  │ algorithm_       │  │ op_registry.c    │                   │ │
│ │  │   runner.c       │  │ • Register algos │                   │ │
│ │  │ • Orchestration  │  │ • Lookup by ID   │                   │ │
│ │  │ • Buffer mgmt    │  │                  │                   │ │
│ │  │ • Verification   │  │                  │                   │ │
│ │  └──────────────────┘  └──────────────────┘                   │ │
│ │                                                                 │ │
│ │  Depends on: include/, platform/, utils/                       │ │
│ └─────────────────────────────────────────────────────────────────┘ │
│                                                                       │
│ ┌─────────────────────────────────────────────────────────────────┐ │
│ │               PLATFORM LAYER (OpenCL Abstraction)               │ │
│ │                     (src/platform/)                             │ │
│ │                                                                 │ │
│ │  ┌──────────────────┐  ┌──────────────────┐                   │ │
│ │  │opencl_utils.c/.h │  │cache_manager.c/.h│                   │ │
│ │  │ • Platform init  │  │ • Binary cache   │                   │ │
│ │  │ • Kernel build   │  │ • Golden cache   │                   │ │
│ │  │ • Execution      │  │                  │                   │ │
│ │  └──────────────────┘  └──────────────────┘                   │ │
│ │                                                                 │ │
│ │  ┌──────────────────────────────┐                              │ │
│ │  │ cl_extension_api.c/.h        │                              │ │
│ │  │ • Custom host API framework  │                              │ │
│ │  └──────────────────────────────┘                              │ │
│ │                                                                 │ │
│ │  Depends on: include/, utils/                                  │ │
│ └─────────────────────────────────────────────────────────────────┘ │
│                                                                       │
│ ┌─────────────────────────────────────────────────────────────────┐ │
│ │              UTILITIES LAYER (Infrastructure)                   │ │
│ │                     (src/utils/)                                │ │
│ │                                                                 │ │
│ │  ┌──────────────────┐  ┌──────────────────┐                   │ │
│ │  │  config.c/.h     │  │  image_io.c/.h   │                   │ │
│ │  │ • INI parsing    │  │ • Image I/O      │                   │ │
│ │  │ • Buffer config  │  │ • Raw format     │                   │ │
│ │  └──────────────────┘  └──────────────────┘                   │ │
│ │                                                                 │ │
│ │  ┌──────────────────┐                                          │ │
│ │  │  verify.c        │                                          │ │
│ │  │ • Verification   │                                          │ │
│ │  │   impls          │                                          │ │
│ │  └──────────────────┘                                          │ │
│ │                                                                 │ │
│ │  Depends on: include/utils/                                    │ │
│ └─────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
                                    │
┌─────────────────────────────────────────────────────────────────────┐
│                          SYSTEM LAYER                                │
│  • OpenCL Runtime  • libc (MISRA-C)  • File System                  │
└─────────────────────────────────────────────────────────────────────┘
```

**Key Design Principles**:
- **Layer separation**: Core, Platform, Utils separated into `src/` subdirectories
- **Public API**: `include/` contains stable public API (interfaces only)
- **User code**: `examples/` depends ONLY on `include/` (no internal headers)
- **Library boundary**: Everything in `src/` packaged as library
- **SDK-ready**: Can distribute library + headers, users implement algorithms

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
│                    (include/op_interface.h)                       │
│                        [PUBLIC API]                               │
│                                                                   │
│  typedef struct {                                                 │
│      char name[64], id[32];       /* Display & ID */              │
│      void (*reference_impl)(OpParams*);  /* C reference ONLY */   │
│  } Algorithm;                                                     │
│                                                                   │
│  NOTE: kernel_args and verification are CONFIG-DRIVEN via INI:   │
│  - [kernel.v*] kernel_args = {...}  → sets OpenCL kernel args    │
│  - [verification] tolerance, error_rate_threshold → verification │
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
│   DILATE 3x3     │  │  GAUSSIAN 5x5    │  │  [USER ALGO]     │
│ (examples/dilate)│  │(examples/gaussian│  │ (examples/...)   │
│                  │  │                  │  │                  │
│  reference_impl: │  │  reference_impl: │  │  reference_impl: │
│  • CPU dilate    │  │  • CPU gaussian  │  │  • CPU impl      │
│                  │  │                  │  │                  │
│  CONFIG-DRIVEN:  │  │  CONFIG-DRIVEN:  │  │  CONFIG-DRIVEN:  │
│  • kernel_args   │  │  • kernel_args   │  │  • kernel_args   │
│  • verification  │  │  • verification  │  │  • verification  │
│                  │  │                  │  │                  │
│  Uses ONLY:      │  │  Uses ONLY:      │  │  Uses ONLY:      │
│  include/*.h     │  │  include/*.h     │  │  include/*.h     │
│  include/utils/  │  │  include/utils/  │  │  include/utils/  │
└──────────────────┘  └──────────────────┘  └──────────────────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
              ┌───────────────────────────────┐
              │   ALGORITHM REGISTRY          │
              │  (src/core/op_registry.c)     │
              │      [LIBRARY CODE]           │
              │                               │
              │  RegisterAlgorithm() API      │
              │  GetAlgorithm() lookup        │
              └───────────────────────────────┘
                              │
                              ▼
              ┌───────────────────────────────┐
              │   AUTO REGISTRATION           │
              │  (src/core/auto_registry.c)   │
              │    [GENERATED BY SCRIPT]      │
              │                               │
              │  extern Algorithm dilate...;  │
              │  extern Algorithm gaussian...;│
              │                               │
              │  AutoRegisterAlgorithms() {   │
              │    RegisterAlgorithm(&dilate);│
              │    RegisterAlgorithm(&gauss); │
              │  }                            │
              └───────────────────────────────┘
                              │
                              ▼
              Used by main.c for execution
```

---

## 5. Per-Algorithm Directory Structure

```
examples/gaussian/                   ← User algorithm module (NOT in library)
│
├── c_ref/                           ← CPU reference implementation
│   └── gaussian5x5_ref.c
│       └── Gaussian5x5Ref()         → C reference impl ONLY!
│                                      (verification & kernel args are config-driven)
│
└── cl/                              ← GPU kernel variants
    ├── gaussian0.cl                 → Kernel variant 0
    ├── gaussian1.cl                 → Kernel variant 1 (optimized)
    └── gaussian2.cl                 → Kernel variant 2 (separable)

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

# Verification settings (config-driven, no C code needed!)
[verification]
tolerance = 1                        ← Max per-pixel difference allowed
error_rate_threshold = 0.001         ← Max % of pixels that can differ

# Kernel variants (supports multiple variants with different signatures)
[kernel.v0]
host_type = standard
kernel_file = examples/gaussian/cl/gaussian0.cl
kernel_function = gaussian5x5
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
kernel_args = {                      ← Config-driven kernel args!
    {input, input_image_id}
    {output, output}
    {int, src_width}
    {int, src_height}
    {buffer, kernel_x}
    {buffer, kernel_y}
}

[kernel.v1]
host_type = cl_extension
kernel_file = examples/gaussian/cl/gaussian1.cl
kernel_function = gaussian5x5_optimized
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
│  kernel_file = examples/gaussian/cl/gaussian0.cl             │
│  kernel_function = gaussian5x5                               │
│  global_work_size = 1920,1088                                │
│  local_work_size = 16,16                                     │
│                                                               │
│  [kernel.v1]                  ← Kernel variant 1 (auto)      │
│  host_type = cl_extension                                    │
│  kernel_file = examples/gaussian/cl/gaussian1.cl             │
│  kernel_function = gaussian5x5_optimized                     │
│  global_work_size = 1920,1088                                │
│  local_work_size = 16,16                                     │
│                                                               │
│  [verification]               ← Verification config (NEW!)   │
│  tolerance = 1                  # Max per-pixel difference   │
│  error_rate_threshold = 0.001   # Max % pixels can differ    │
└───────────────────────────────────────────────────────────────┘
                            │
                            │ parse_config()
                            ▼
┌───────────────────────────────────────────────────────────────┐
│              CONFIG PARSER (src/utils/config.c)               │
│                      [IN LIBRARY]                             │
│                                                               │
│  1. Parse [image] section                                    │
│     → ImageConfig (dims, I/O paths)                          │
│                                                               │
│  2. Parse [buffer.*] sections                                │
│     → BufferConfig[] (type, size, source)                    │
│                                                               │
│  3. Parse [kernel.*] sections                                │
│     → KernelConfig[] (file, function, work sizes,            │
│                       host_type, kernel_variant, kernel_args)│
│                                                               │
│  4. Parse [verification] section                             │
│     → VerificationConfig (tolerance, error_rate_threshold)   │
│                                                               │
│  5. Validate & store in AlgorithmConfig                      │
└───────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────────┐
│             OPENCL EXECUTION (src/platform/opencl_utils.c)    │
│                      [IN LIBRARY]                             │
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

## 10. Build System & SDK Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│                      BUILD WORKFLOW (CMAKE)                     │
└─────────────────────────────────────────────────────────────────┘

                  ┌────────────────────────────────┐
                  │    scripts/build.sh            │
                  │  Two-stage SDK workflow:       │
                  │  1. --lib: Build library only  │
                  │  2. (default): Build executable│
                  └──────────────┬─────────────────┘
                                 │
                  ┌──────────────┴─────────────────┐
                  │  scripts/generate_registry.sh  │
                  │  • Scans examples/**/c_ref/*.c │
                  │  • Auto-generates registry     │
                  │  • Creates auto_registry.c     │
                  └──────────────┬─────────────────┘
                                 │
                                 ▼
                      ┌────────────────┐
                      │ CMakeLists.txt │
                      └────────┬───────┘
                               │
           ┌───────────────────┼───────────────────┐
           ▼                   ▼                   ▼
    ┌─────────────┐    ┌──────────────┐    ┌──────────┐
    │  LIBRARY    │    │  EXAMPLES    │    │  MAIN    │
    │  TARGET     │    │  (User Code) │    │  (App)   │
    └─────────────┘    └──────────────┘    └──────────┘
           │                   │                   │
           │                   │                   │
     ┌─────┴──────┐      ┌─────┴──────┐      ┌─────┴──────┐
     ▼            ▼      ▼            ▼      ▼            ▼
┌─────────┐  ┌────────┐ ┌─────────┐ ┌─────┐ ┌──────┐  ┌────────┐
│  core/  │  │platform│ │examples/│ │auto_│ │main.c│  │examples│
│  *.c    │  │  *.c   │ │dilate/  │ │reg. │ │      │  │ (link) │
└─────────┘  └────────┘ └─────────┘ └─────┘ └──────┘  └────────┘
     │            │           │         │         │         │
     └────────────┴───────────┼─────────┘         │         │
                              │                   │         │
                              ▼                   ▼         ▼
                   ┌────────────────────┐  ┌──────────────────┐
                   │   LIBRARY BUILD    │  │  EXECUTABLE      │
                   │ libopencl_imgproc  │  │  opencl_host     │
                   │   .so / .a         │  │                  │
                   └────────┬───────────┘  └──────────────────┘
                            │
                            ▼
                   ┌────────────────────┐
                   │  Copy to lib/      │
                   │  (for SDK release) │
                   └────────────────────┘

SDK BUILD WORKFLOW:
───────────────────

Step 1: Build Library (ONCE)
  $ ./scripts/build.sh --lib

  → Builds: libopencl_imgproc.so (or .dylib)
  → Contains: core/ + platform/ + utils/
  → Copies to: lib/ (for distribution)
  → Does NOT build: examples or executable

Step 2: Build Executable (when examples change)
  $ ./scripts/build.sh

  → Generates: auto_registry.c from examples/
  → Compiles: main.c + examples/*.c + auto_registry.c
  → Links: against libopencl_imgproc
  → Creates: build/opencl_host
  → Library: NOT rebuilt (unchanged)

BENEFITS:
  ✓ Library built once (core/platform/utils rarely change)
  ✓ Executable rebuilds quickly (only examples changed)
  ✓ Clean separation for SDK distribution
  ✓ Customers can add algorithms without library source
  ✓ lib/ directory ready for release packaging
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
   │   ├─► Scans examples/ for *_ref.c files
   │   │   - Finds: dilate3x3_ref.c, gaussian5x5_ref.c
   │   │
   │   ├─► Extracts algorithm IDs and display names
   │   │   - Reads config/*.ini files
   │   │   - Extracts [image] display_name fields
   │   │
   │   └─► Generates src/core/auto_registry.c
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
      "kernel_file": "examples/gaussian/cl/gaussian0.cl",
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
   examples/<algo_name>/
   ├── c_ref/
   │   └── <algo_name>_ref.c  ← C reference + callbacks + registration
   └── cl/
       └── <algo_name>0.cl    ← OpenCL kernel template

3. Generate template files:
   ├─► c_ref/*.c contains:
   │   - reference_impl() function (calls C reference)
   │   - verify_result() function (tolerance-based comparison)
   │   - set_kernel_args() function (sets CL kernel arguments)
   │   - Algorithm struct registration
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
   ✓ Algorithm structure created in examples/<algo_name>/
   ✓ Configuration created in config/<algo_name>.ini

   NEXT STEPS:
   1. Implement C reference in examples/<algo_name>/c_ref/
   2. Implement OpenCL kernel in examples/<algo_name>/cl/
   3. Update set_kernel_args() for your buffer layout
   4. Run: ./scripts/build.sh && ./scripts/run.sh
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

## 12. SDK Distribution Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                  SDK PACKAGE STRUCTURE                          │
│              (scripts/create_sdk.sh --shared)                   │
└─────────────────────────────────────────────────────────────────┘

opencl-imgproc-sdk-1.0.0/
├── lib/
│   └── libopencl_imgproc.so          ← Compiled library (customers link)
│
├── include/                           ← Public API (customers include)
│   ├── op_interface.h                 → Algorithm interface
│   ├── op_registry.h                  → Registration macros
│   ├── algorithm_runner.h             → Forward declarations only
│   └── utils/
│       ├── safe_ops.h                 → Safe arithmetic
│       └── verify.h                   → Verification functions
│
├── examples/                          ← Sample algorithms (reference)
│   ├── main.c.example                 → Example main.c
│   ├── dilate/
│   │   ├── c_ref/dilate3x3_ref.c
│   │   └── cl/dilate0.cl
│   └── gaussian/
│       ├── c_ref/gaussian5x5_ref.c
│       └── cl/gaussian*.cl
│
├── tools/
│   └── generate_registry.sh          ← For customers to use
│
└── docs/
    ├── README.md                      → SDK quick start
    ├── API_REFERENCE.md               → Public API documentation
    └── SDK_PACKAGING.md               → Usage guide

CUSTOMER WORKFLOW:
──────────────────

1. Customer receives SDK package
2. Customer implements algorithm in their project:

   customer-app/
   ├── sdk/                            ← Your SDK
   │   ├── lib/libopencl_imgproc.so
   │   └── include/
   ├── main.c                          ← From SDK examples
   └── my_algorithms/
       └── edge_detect/
           └── c_ref/edge_detect_ref.c

3. Customer builds:

   $ cd customer-app
   $ ../sdk/tools/generate_registry.sh my_algorithms auto_registry.c
   $ gcc main.c auto_registry.c my_algorithms/*/*.c \
       -I./sdk/include -L./sdk/lib -lopencl_imgproc \
       -framework OpenCL -o my_app

4. Customer runs:

   $ ./my_app config/edge_detect.ini 0

WHAT CUSTOMERS GET:
  ✓ Compiled library (.so)
  ✓ Public API headers
  ✓ Example main.c
  ✓ Sample algorithms
  ✓ Registration script

WHAT CUSTOMERS CANNOT SEE:
  ✗ Core implementation (algorithm_runner.c)
  ✗ Platform implementation (opencl_utils.c)
  ✗ Utils implementation (config.c, image_io.c)
  ✗ Internal headers (src/*/*)
```

---

## 13. Clean Architecture Compliance

```
┌─────────────────────────────────────────────────────────────────┐
│              CLEAN ARCHITECTURE ANALYSIS                        │
└─────────────────────────────────────────────────────────────────┘

LAYER HIERARCHY (Outer → Inner):
─────────────────────────────────

┌─────────────────┐
│   examples/     │  User Algorithm Implementations (Outermost)
└────────┬────────┘  Instability: 1.00 (most unstable)
         │ depends on
         ▼
┌─────────────────┐
│     main        │  Application Entry Point
└────────┬────────┘  Instability: 1.00 (most unstable)
         │ depends on
         ▼
┌─────────────────┐
│   include/      │  Public API (Stable Abstractions)
│ include/utils/  │  - Interfaces and contracts
└────────┬────────┘  Instability: 0.00 (most stable)
         │ used by
         ▼
┌─────────────────┐
│     core/       │  Business Logic (Use Cases)
└────────┬────────┘  - Algorithm execution pipeline
         │ depends on  Instability: 1.00
         ▼
┌─────────────────┐
│   platform/     │  Platform Abstraction (Infrastructure)
└────────┬────────┘  - OpenCL platform management
         │ depends on  Instability: 0.50
         ▼
┌─────────────────┐
│    utils/       │  Utilities & Entities (Innermost)
└─────────────────┘  - Configuration, I/O, verification
                     Instability: 1.00

DEPENDENCY FLOW:
────────────────
  examples    → include, include/utils
  main        → include, include/utils, platform
  include     → (no dependencies)
  include/utils → (no dependencies)
  core        → include, include/utils, platform
  platform    → include, include/utils
  utils       → include, include/utils

COMPLIANCE:
───────────
✅ Dependency Rule: All dependencies point inward
✅ Stable Abstractions: include/ is most stable (I=0.00)
✅ Business Logic Isolation: core/ independent of examples
✅ Plugin Architecture: examples/ are plugins
✅ SDK Ready: Public API separated from implementation

PRINCIPLE ADHERENCE:
────────────────────
✅ Dependency Inversion Principle
✅ Open/Closed Principle (extend via examples/)
✅ Single Responsibility Principle
✅ Interface Segregation Principle
✅ Stable Abstractions Principle
```

---

## Summary

This architecture provides:

1. **Modularity** - Library packaging with clear boundaries
   - Library: core/ + platform/ + utils/
   - Public API: include/
   - User code: examples/
   - Application: src/main.c

2. **SDK Distribution** - Easy to package and distribute
   - Build library once: `./scripts/build.sh --lib`
   - Distribute: lib/ + include/ + tools/
   - Customers extend: Add algorithms without library source

3. **Clean Architecture** - Compliant with Clean Architecture principles
   - Stable abstractions (include/ I=0.00)
   - Dependency inversion (all deps point inward)
   - Plugin architecture (examples/ as plugins)
   - Clear layer boundaries

4. **Extensibility** - Add algorithms without modifying library
   - Auto-discovery system
   - Registration script for customers
   - No manual registration required

5. **Flexibility** - Per-algorithm INI files with custom buffers
   - Support for file-backed and empty buffers
   - Multiple kernel variants per algorithm
   - Auto-derived variant IDs from section names

6. **Performance** - Binary caching accelerates iteration (50-60x)
   - Kernel binary caching (~100x faster compilation)
   - Golden sample caching (~50x faster CPU reference)
   - Automated benchmarking for performance tracking

7. **Safety** - MISRA-C compliant (static allocation, safe arithmetic)
   - No dynamic allocation in hot paths
   - Bounds checking and safe operations
   - Deterministic memory layout

8. **Verification** - CPU reference validates GPU correctness
   - Algorithm-specific tolerance levels
   - Automatic verification on every run
   - Detailed error reporting

9. **Scalability** - Multiple algorithms, multiple variants per algorithm
   - Clean variant management system
   - Support for different optimization strategies
   - API selection (standard vs. cl_extension)

10. **Automation** - Comprehensive tooling infrastructure
    - **Build automation**: Auto-registry generation and incremental compilation
    - **Benchmarking**: Automated performance testing across all variants
    - **Metadata extraction**: JSON export for documentation and analysis
    - **Algorithm creation**: Template generator for new algorithms
    - **Test data generation**: Synthetic image and kernel weight generation

11. **Maintainability** - Clear separation of concerns
    - Library code: src/core/, src/platform/, src/utils/
    - Public API: include/
    - User code: examples/
    - Build tools: scripts/

The design follows professional software engineering practices suitable for client demonstrations, research, and production deployment. The comprehensive automation infrastructure enables rapid algorithm development, testing, and performance optimization.
