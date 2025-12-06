# OpenCL Codebase Architecture (v1 - SDK-Ready)

Visual diagrams to understand the SDK-ready system architecture and execution flow.

**Version**: 1.0 (SDK-Ready Architecture)
**Date**: 2025-12-06
**Changes from v0**: Clean Architecture refactoring with library/SDK packaging support

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

**Key Changes from v0**:
- Library separation: Core + Platform + Utils packaged as `libopencl_imgproc`
- Algorithms moved to `examples/` (user code, not part of library)
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
│  │ • Verification  │  │ • Verification  │                          │
│  │ • Set args      │  │ • Set args      │                          │
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
│  │   void (*reference_impl)(OpParams*);                           │  │
│  │   int (*verify_result)(OpParams*, float*);                     │  │
│  │   int (*set_kernel_args)(cl_kernel, cl_mem, cl_mem, OpParams);│  │
│  │ } Algorithm;                                                   │  │
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

**Key Changes from v0**:
- **Layer separation**: Core, Platform, Utils separated into `src/` subdirectories
- **Public API**: `include/` contains stable public API (interfaces only)
- **User code**: `examples/` depends ONLY on `include/` (no internal headers)
- **Library boundary**: Everything in `src/` packaged as library
- **SDK-ready**: Can distribute library + headers, users implement algorithms

---

## 3. Execution Flow (Single Algorithm Run)

*[Same as v0 - no changes to execution flow]*

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
│   DILATE 3x3     │  │  GAUSSIAN 5x5    │  │  [USER ALGO]     │
│ (examples/dilate)│  │(examples/gaussian│  │ (examples/...)   │
│                  │  │                  │  │                  │
│  reference_impl: │  │  reference_impl: │  │  reference_impl: │
│  • CPU dilate    │  │  • CPU gaussian  │  │  • CPU impl      │
│                  │  │                  │  │                  │
│  verify_result:  │  │  verify_result:  │  │  verify_result:  │
│  • Exact match   │  │  • ±1 tolerance  │  │  • Custom tol    │
│                  │  │                  │  │                  │
│  set_kernel_args:│  │  set_kernel_args:│  │  set_kernel_args:│
│  • input, output │  │  • input, output │  │  • [custom]      │
│  • w, h, stride  │  │  • buffers, w, h │  │                  │
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

**Key Changes from v0**:
- Algorithms in `examples/` directory (user code)
- Registry in library (`src/core/op_registry.c`)
- Auto-registration generated by script
- User algorithms depend ONLY on public API (`include/`)

---

## 5. Per-Algorithm Directory Structure

```
examples/gaussian/                   ← User algorithm module (NOT in library)
│
├── c_ref/                           ← CPU reference implementation
│   └── gaussian5x5_ref.c
│       ├── Gaussian5x5Ref()         → C reference impl
│       ├── Gaussian5x5Verify()      → ±1 tolerance verification
│       ├── Gaussian5x5SetArgs()     → Set kernel arguments
│       └── REGISTER_ALGORITHM()     → Auto-registration macro
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

# Kernel variants (supports multiple variants with different signatures)
[kernel.v0]
host_type = standard
kernel_file = examples/gaussian/cl/gaussian0.cl
kernel_function = gaussian5x5
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

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

**Key Changes from v0**:
- Algorithms in `examples/` instead of `src/`
- Simpler structure: `c_ref/` and `cl/` subdirectories
- All algorithm code in single `.c` file
- Uses `REGISTER_ALGORITHM()` macro for registration
- Kernel paths updated to `examples/*/cl/`

---

## 6. Data Flow Through the System

*[Same as v0 - no changes to data flow]*

---

## 7. Configuration System Flow

*[Same as v0 - paths updated to examples/]*

```
┌───────────────────────────────────────────────────────────────┐
│                 config/gaussian5x5.ini                        │
│                                                               │
│  [image]                      ← Image dimensions & I/O       │
│  input = test_data/input.bin                                 │
│  src_width = 1920                                            │
│  src_height = 1080                                           │
│                                                               │
│  [kernel.v0]                  ← Kernel variant 0             │
│  host_type = standard                                        │
│  kernel_file = examples/gaussian/cl/gaussian0.cl             │
│  kernel_function = gaussian5x5                               │
│  global_work_size = 1920,1088                                │
│  local_work_size = 16,16                                     │
└───────────────────────────────────────────────────────────────┘
                            │
                            │ parse_config() [LIBRARY]
                            ▼
┌───────────────────────────────────────────────────────────────┐
│              CONFIG PARSER (src/utils/config.c)               │
│                      [IN LIBRARY]                             │
│                                                               │
│  1. Parse [image] section                                    │
│  2. Parse [buffer.*] sections                                │
│  3. Parse [kernel.*] sections                                │
│  4. Validate & store in Config struct                        │
└───────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────────┐
│             OPENCL EXECUTION (src/platform/opencl_utils.c)    │
│                      [IN LIBRARY]                             │
│                                                               │
│  1. Create buffers (from ImageConfig)                        │
│  2. Create custom buffers (from BufferConfig[])              │
│  3. Build kernel (from KernelConfig)                         │
│  4. Set kernel arguments via algorithm callback              │
│  5. Execute kernel with configured work sizes                │
└───────────────────────────────────────────────────────────────┘
```

**Key Changes from v0**:
- Config parser in library (`src/utils/config.c`)
- Kernel paths point to `examples/` directory

---

## 8. Caching System Architecture

*[Same as v0 - no changes to caching system]*

---

## 9. Memory Architecture (MISRA-C Compliant)

*[Same as v0 - no changes to memory architecture]*

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

**Key Changes from v0**:
- **CMake-based**: Replaced Makefile with CMakeLists.txt
- **Library target**: `libopencl_imgproc` (SHARED or STATIC)
- **Two-stage build**: `--lib` for library, default for executable
- **SDK workflow**: Build library once, rebuild executable when examples change
- **lib/ directory**: Stores release artifacts

---

## 11. SDK Distribution Architecture

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

**New in v1**:
- SDK packaging support via `scripts/create_sdk.sh`
- Binary distribution model
- Customer workflow documentation
- Clear separation of public API vs internal implementation

---

## 12. Clean Architecture Compliance

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

**New in v1**:
- Formal Clean Architecture analysis
- Stability metrics
- Layer separation verification
- Documented compliance

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
   - Same as v0 (no changes)

6. **Performance** - Binary caching accelerates iteration (50-60x)
   - Same as v0 (no changes)

7. **Safety** - MISRA-C compliant (static allocation, safe arithmetic)
   - Same as v0 (no changes)

8. **Verification** - CPU reference validates GPU correctness
   - Same as v0 (no changes)

9. **Scalability** - Multiple algorithms, multiple variants
   - Same as v0 (no changes)

10. **Maintainability** - Clear separation of concerns
    - Library code: src/core/, src/platform/, src/utils/
    - Public API: include/
    - User code: examples/
    - Build tools: scripts/

The v1 architecture builds on v0 with Clean Architecture refactoring and SDK packaging support, making it production-ready for commercial distribution while maintaining all existing features.
