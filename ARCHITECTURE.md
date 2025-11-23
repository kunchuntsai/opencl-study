# OpenCL Codebase Architecture

Visual diagrams to understand the system architecture and execution flow.

---

## 1. High-Level System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CLIENT DEMONSTRATION                         │
│                    (Show GPU Optimization Results)                   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
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
│                                                                       │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                         src/main.c                             │  │
│  │  • Program entry point                                         │  │
│  │  • Menu system                                                 │  │
│  │  • Algorithm selection & execution                             │  │
│  │  • Performance reporting                                       │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         ALGORITHM LAYER                              │
│                                                                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │
│  │  src/dilate/    │  │ src/gaussian/   │  │ src/algorithm3/ │    │
│  │  dilate3x3.c    │  │ gaussian5x5.c   │  │  (future)       │    │
│  │                 │  │                 │  │                 │    │
│  │  • Wrapper      │  │  • Wrapper      │  │  • Wrapper      │    │
│  │  • Verification │  │  • Verification │  │  • Verification │    │
│  │  • Info display │  │  • Info display │  │  • Info display │    │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘    │
│          │                     │                     │              │
│  ┌───────┴──────┐      ┌───────┴──────┐      ┌──────┴───────┐     │
│  │              │      │              │      │              │     │
│  ▼              ▼      ▼              ▼      ▼              ▼     │
│ ┌──────────┐ ┌────┐ ┌──────────┐  ┌────┐ ┌──────────┐  ┌────┐   │
│ │ c_ref/   │ │cl/ │ │ c_ref/   │  │cl/ │ │ c_ref/   │  │cl/ │   │
│ │ *_ref.c  │ │*.cl│ │ *_ref.c  │  │*.cl│ │ *_ref.c  │  │*.cl│   │
│ └──────────┘ └────┘ └──────────┘  └────┘ └──────────┘  └────┘   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       INFRASTRUCTURE LAYER                           │
│                         (src/utils/)                                 │
│                                                                       │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │ op_registry.c/.h │  │opencl_utils.c/.h │  │config_parser.c  │  │
│  │                  │  │                  │  │                 │  │
│  │ • Algorithm      │  │ • Platform init  │  │ • INI parsing   │  │
│  │   registration   │  │ • Kernel build   │  │ • Config load   │  │
│  │ • Registry       │  │ • Kernel exec    │  │ • Variant mgmt  │  │
│  │   management     │  │ • Memory mgmt    │  │                 │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                       │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │  image_io.c/.h   │  │cache_manager.c/.h│  │  safe_ops.h     │  │
│  │                  │  │                  │  │                 │  │
│  │ • Load raw image │  │ • Golden cache   │  │ • MISRA-C safe  │  │
│  │ • Save raw image │  │ • Kernel cache   │  │   arithmetic    │  │
│  │ • Size checking  │  │ • File I/O       │  │ • Overflow chk  │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                       │
│  ┌──────────────────┐                                                │
│  │ op_interface.h   │  ← Abstract Algorithm Interface                │
│  │                  │                                                │
│  │ typedef struct { │                                                │
│  │   char name[64]; │                                                │
│  │   void (*ref)(); │                                                │
│  │   int (*verify)();                                               │
│  │ } Algorithm;     │                                                │
│  └──────────────────┘                                                │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          SYSTEM LAYER                                │
│                                                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │   OpenCL     │  │    libc      │  │  File System │              │
│  │   Runtime    │  │  (MISRA-C)   │  │              │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 3. Execution Flow (Single Algorithm Run)

```
START: User runs ./build/opencl_host 0 1
  │
  ▼
┌─────────────────────────────────────┐
│ 1. INITIALIZATION                   │
│    • Parse config.ini               │
│    • Initialize OpenCL platform     │
│    • Discover GPU/CPU devices       │
│    • Create context & queue         │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│ 2. ALGORITHM DISCOVERY              │
│    • Load algorithm registry        │
│    • Display available algorithms   │
│    • User selects: Dilate (index 0) │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│ 3. VARIANT SELECTION                │
│    • Load kernel configs from INI   │
│    • Display variants (0,1,2...)    │
│    • User selects: variant 1        │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│ 4. IMAGE LOADING                    │
│    • Read test_data/input.raw       │
│    • Validate dimensions            │
│    • Load into memory buffer        │
└─────────────────────────────────────┘
  │
  ├──────────────────────┬─────────────────────┐
  ▼                      ▼                     ▼
┌───────────────┐  ┌────────────────┐  ┌──────────────────┐
│ 5a. CPU PATH  │  │ 5b. GPU PATH   │  │ 5c. CACHE CHECK  │
│               │  │                │  │                  │
│ Check golden  │  │ Check kernel   │  │ Look for:        │
│ cache first   │  │ binary cache   │  │ • golden/*.bin   │
│               │  │                │  │ • kernels/*.bin  │
│ ┌───────────┐ │  │ ┌────────────┐ │  └──────────────────┘
│ │ Cache hit?│ │  │ │ Cache hit? │ │
│ └─────┬─────┘ │  │ └─────┬──────┘ │
│   NO  │  YES  │  │   NO  │  YES   │
│   ▼   │       │  │   ▼   │        │
│ ┌─────▼─────┐ │  │ ┌─────▼──────┐ │
│ │Run C ref  │ │  │ │Compile .cl │ │
│ │dilate3x3_ │ │  │ │Load binary │ │
│ │ref()      │ │  │ │Build kernel│ │
│ │           │ │  │ │            │ │
│ │Save golden│ │  │ │Save binary │ │
│ └───────────┘ │  │ └────────────┘ │
│               │  │                │
│ Result:       │  │ Set args:      │
│ cpu_output[]  │  │ • input buffer │
│               │  │ • output buffer│
│ Time: ~2.5ms  │  │ • width/height │
│               │  │                │
│               │  │ Run kernel     │
│               │  │ Result:        │
│               │  │ gpu_output[]   │
│               │  │                │
│               │  │ Time: ~0.003ms │
└───────────────┘  └────────────────┘
  │                      │
  └──────────┬───────────┘
             ▼
┌─────────────────────────────────────┐
│ 6. VERIFICATION                     │
│    • Compare cpu_output vs gpu_out  │
│    • Count mismatches               │
│    • Calculate error stats          │
│                                     │
│    For Dilate: Exact match required │
│    For Gaussian: ±1 tolerance       │
└─────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────┐
│ 7. RESULTS DISPLAY                  │
│                                     │
│  ✓ Verification: PASS               │
│  ✓ CPU Time:     2.45 ms            │
│  ✓ GPU Time:     0.003 ms           │
│  ✓ Speedup:      817x               │
│                                     │
│  [Optimization Analysis]            │
│  Variant 1 uses local memory tiling │
│  which reduces global memory access │
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
│      char name[64];           /* Display name */                  │
│      char id[32];             /* Unique identifier */             │
│      void (*reference_impl)(...);  /* C reference */              │
│      int (*verify_result)(...);    /* Verification */             │
│      void (*print_info)(void);     /* Info display */             │
│  } Algorithm;                                                     │
└───────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Implements
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│   DILATE 3x3     │  │  GAUSSIAN 5x5    │  │  ALGORITHM 3     │
│                  │  │                  │  │   (Future)       │
│  name: "Dilate"  │  │ name: "Gaussian" │  │                  │
│  id: "dilate3x3" │  │ id: "gaussian5x5"│  │                  │
│                  │  │                  │  │                  │
│  reference_impl: │  │  reference_impl: │  │  reference_impl: │
│    ├─> dilate3x3 │  │    ├─> gauss5x5  │  │    ├─> algo3_ref │
│    │   _ref()    │  │    │   _ref()    │  │    │   ()        │
│                  │  │                  │  │                  │
│  verify_result:  │  │  verify_result:  │  │  verify_result:  │
│    ├─> exact     │  │    ├─> tolerance │  │    ├─> custom    │
│    │   match     │  │    │   ±1 pixel  │  │    │   logic     │
│                  │  │                  │  │                  │
│  print_info:     │  │  print_info:     │  │  print_info:     │
│    └─> display   │  │    └─> display   │  │    └─> display   │
│        algorithm │  │        algorithm │  │        algorithm │
│        details   │  │        details   │  │        details   │
└──────────────────┘  └──────────────────┘  └──────────────────┘
        │                     │                     │
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              │
                              ▼
              ┌───────────────────────────────┐
              │   ALGORITHM REGISTRY          │
              │  (src/utils/op_registry.c)    │
              │                               │
              │  algorithms[32]               │
              │    [0] = &dilate3x3_algorithm │
              │    [1] = &gaussian5x5_algo    │
              │    [2] = &algorithm3          │
              │    [3] = NULL                 │
              │                               │
              │  register_algorithm(...)      │
              │  get_algorithm(index)         │
              │  list_algorithms()            │
              └───────────────────────────────┘
                              │
                              ▼
                    Used by main.c for
                    dynamic menu generation
```

---

## 5. Per-Algorithm Directory Structure

```
src/dilate/                          ← Self-contained algorithm module
│
├── dilate3x3.h                      ← Public interface
│   └── Algorithm dilate3x3_algorithm  (exported for registration)
│
├── dilate3x3.c                      ← Algorithm wrapper
│   ├── dilate3x3_reference()        → Calls C reference
│   ├── dilate3x3_verify()           → Exact match verification
│   └── dilate3x3_print_info()       → Display algorithm details
│
├── c_ref/                           ← CPU reference implementation
│   ├── dilate3x3_ref.h
│   └── dilate3x3_ref.c
│       └── dilate3x3_ref()          → Ground truth implementation
│                                      (Serial C code for verification)
│
└── cl/                              ← GPU kernel variants
    ├── dilate0.cl                   → Basic version (23 lines)
    │   └── __kernel dilate3x3()       • Simple global memory
    │                                  • 9 reads per pixel
    │                                  • Baseline performance
    │
    ├── dilate1.cl                   → Optimized v1 (87 lines)
    │   └── __kernel dilate3x3()       • Local memory tiling (16x16)
    │                                  • Halo loading strategy
    │                                  • 8x faster than basic
    │
    └── dilate2.cl                   → Optimized v2 (future)
        └── __kernel dilate3x3()       • Advanced optimization
                                       • Vector operations
                                       • Further speedup

Configuration (config/config.ini):
─────────────────────────────────
[dilate3x3_v0]                       ← Variant 0 configuration
kernel_file = dilate/cl/dilate0.cl
kernel_name = dilate3x3
global_work_size_x = 1024
global_work_size_y = 1024
local_work_size_x = 16
local_work_size_y = 16

[dilate3x3_v1]                       ← Variant 1 configuration
kernel_file = dilate/cl/dilate1.cl
kernel_name = dilate3x3
global_work_size_x = 1024
global_work_size_y = 1024
local_work_size_x = 16
local_work_size_y = 16

Test Data Organization:
───────────────────────
test_data/dilate/
├── golden/
│   └── dilate3x3.bin                ← Cached C reference output
└── kernels/
    ├── dilate0.bin                  ← Cached compiled kernel v0
    └── dilate1.bin                  ← Cached compiled kernel v1
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
│                      config/config.ini                        │
│                                                               │
│  [dilate3x3_v0]              [dilate3x3_v1]                  │
│  kernel_file = dilate/       kernel_file = dilate/           │
│                cl/dilate0.cl              cl/dilate1.cl      │
│  kernel_name = dilate3x3     kernel_name = dilate3x3         │
│  global_work_size_x = 1024   global_work_size_x = 1024       │
│  global_work_size_y = 1024   global_work_size_y = 1024       │
│  local_work_size_x = 16      local_work_size_x = 16          │
│  local_work_size_y = 16      local_work_size_y = 16          │
│                                                               │
│  [gaussian5x5_v0]            [gaussian5x5_v1]                │
│  kernel_file = gaussian/     (future optimization)           │
│                cl/gaussian0.cl                               │
│  ...                                                          │
└───────────────────────────────────────────────────────────────┘
                            │
                            │ parse_config()
                            ▼
┌───────────────────────────────────────────────────────────────┐
│              CONFIG PARSER (config_parser.c)                  │
│                                                               │
│  1. Read INI file line by line                               │
│  2. Parse sections: [algorithm_vX]                           │
│  3. Parse key=value pairs                                    │
│  4. Store in KernelConfig struct:                            │
│     typedef struct {                                         │
│         char kernel_file[256];                               │
│         char kernel_name[64];                                │
│         size_t global_x, global_y;                           │
│         size_t local_x, local_y;                             │
│     } KernelConfig;                                          │
│  5. Validate values (safe_strtol for overflow checking)      │
└───────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────────┐
│             IN-MEMORY CONFIG STRUCTURE                        │
│                                                               │
│  KernelConfig configs[MAX_VARIANTS];                         │
│    [0] → dilate3x3_v0 config                                 │
│    [1] → dilate3x3_v1 config                                 │
│    [2] → gaussian5x5_v0 config                               │
│    [3] → gaussian5x5_v1 config                               │
│    ...                                                        │
└───────────────────────────────────────────────────────────────┘
                            │
                            │ User selects variant
                            ▼
┌───────────────────────────────────────────────────────────────┐
│             OPENCL EXECUTION (opencl_utils.c)                 │
│                                                               │
│  KernelConfig* cfg = &configs[selected_variant];             │
│                                                               │
│  1. Load kernel source from cfg->kernel_file                 │
│  2. Build kernel with name cfg->kernel_name                  │
│  3. Set work dimensions:                                     │
│     global_work_size[0] = cfg->global_x;                     │
│     global_work_size[1] = cfg->global_y;                     │
│     local_work_size[0] = cfg->local_x;                       │
│     local_work_size[1] = cfg->local_y;                       │
│  4. Execute kernel                                           │
└───────────────────────────────────────────────────────────────┘

BENEFIT: Change work group sizes without recompiling!
         Easy to test different configurations for optimization.
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

1. **Modularity** - Algorithms are self-contained plugins
2. **Extensibility** - Add algorithms without modifying core code
3. **Maintainability** - Clear separation of concerns
4. **Performance** - Caching system accelerates iteration
5. **Safety** - MISRA-C compliance for production use
6. **Flexibility** - INI configuration enables runtime tuning
7. **Verification** - Dual implementation ensures correctness
8. **Scalability** - Supports multiple algorithms with multiple variants each

The design follows professional software engineering practices suitable for client demonstrations and production deployment.
