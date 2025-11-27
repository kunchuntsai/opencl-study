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
│                   (Auto-discovered and registered)                   │
│                                                                       │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │
│  │  src/dilate/    │  │ src/gaussian/   │  │ src/algorithm3/ │    │
│  │                 │  │                 │  │  (future)       │    │
│  │  c_ref/         │  │  c_ref/         │  │  c_ref/         │    │
│  │  ├─ *_ref.c     │  │  ├─ *_ref.c     │  │  ├─ *_ref.c     │    │
│  │  │  (reference) │  │  │  (reference) │  │  │  (reference) │    │
│  │  │  (verify)    │  │  │  (verify)    │  │  │  (verify)    │    │
│  │  │  (set_args)  │  │  │  (set_args)  │  │  │  (set_args)  │    │
│  │  │              │  │  │  (create_buf)│  │  │              │    │
│  │  │              │  │  │  (destroy_buf)│  │              │    │
│  │  cl/            │  │  cl/            │  │  cl/            │    │
│  │  └─ *.cl        │  │  └─ *.cl        │  │  └─ *.cl        │    │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘    │
│                                                                       │
│  ⚙️ Build system automatically:                                      │
│     • Scans *_ref.c files                                            │
│     • Detects function signatures                                    │
│     • Generates src/utils/auto_registry.c                            │
│     • Registers all algorithms before main()                         │
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
│  │   char name[64]; │  • Display name                                │
│  │   char id[32];   │  • Unique identifier                           │
│  │   (*ref)(OpParams*); • C reference                                │
│  │   (*verify)(OpParams*, float*); • Verification                    │
│  │   (*create_buffers)(...);  • Optional custom buffers              │
│  │   (*destroy_buffers)(...); • Optional cleanup                     │
│  │   (*set_kernel_args)(...); • Kernel arg setup (required)          │
│  │ } Algorithm;     │                                                │
│  └──────────────────┘                                                │
│                                                                       │
│  ┌──────────────────┐                                                │
│  │auto_registry.c/.h│  ← Auto-generated registry                     │
│  │                  │                                                │
│  │ • Generated by   │    scripts/generate_registry.sh                │
│  │ • Scans *_ref.c  │                                                │
│  │ • Auto-registers │                                                │
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
│ Result:       │  │ ┌────────────────┐
│ cpu_output[]  │  │ │Custom Buffers? │
│               │  │ └───────┬────────┘
│ Time: ~2.5ms  │  │    YES  │  NO
│               │  │   ▼     │
│               │  │ ┌───────▼──────┐ │
│               │  │ │create_buffers│ │
│               │  │ │  (weights,   │ │
│               │  │ │   temp bufs) │ │
│               │  │ └──────┬───────┘ │
│               │  │        ▼         │
│               │  │ ┌────────────────┐
│               │  │ │ set_kernel_args│
│               │  │ │ • input buffer │
│               │  │ │ • output buffer│
│               │  │ │ • custom bufs  │
│               │  │ │ • width/height │
│               │  │ └──────┬─────────┘
│               │  │        ▼
│               │  │ ┌────────────────┐
│               │  │ │  Run kernel    │
│               │  │ └──────┬─────────┘
│               │  │        ▼
│               │  │ ┌────────────────┐
│               │  │ │destroy_buffers │
│               │  │ │ (cleanup)      │
│               │  │ └──────┬─────────┘
│               │  │        ▼
│               │  │ Result:
│               │  │ gpu_output[]
│               │  │
│               │  │ Time: ~0.003ms
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
│      char name[64];                 /* Display name */            │
│      char id[32];                   /* Unique identifier */       │
│      void (*reference_impl)(const OpParams*);  /* C reference */  │
│      int (*verify_result)(const OpParams*, float*); /* Verify */  │
│                                                                   │
│      /* Custom buffer management (optional) */                    │
│      CreateBuffersFunc create_buffers;    /* NULL if unused */    │
│      DestroyBuffersFunc destroy_buffers;  /* NULL if unused */    │
│                                                                   │
│      /* Kernel argument setup (REQUIRED) */                       │
│      SetKernelArgsFunc set_kernel_args;   /* Sets all args */     │
│  } Algorithm;                                                     │
│                                                                   │
│  /* Flexible parameter structure */                               │
│  typedef struct {                                                 │
│      unsigned char* input;       /* Input image */                │
│      unsigned char* output;      /* Output buffer */              │
│      unsigned char* ref_output;  /* Reference output */           │
│      unsigned char* gpu_output;  /* GPU output */                 │
│      int src_width, src_height;  /* Source dimensions */          │
│      int dst_width, dst_height;  /* Dest dimensions */            │
│      BorderMode border_mode;     /* Border handling */            │
│      void* algo_params;          /* Algorithm-specific params */  │
│  } OpParams;                                                      │
└───────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Implements
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│   DILATE 3x3     │  │  GAUSSIAN 5x5    │  │  ALGORITHM 3     │
│  (Standard)      │  │ (Custom Buffers) │  │   (Future)       │
│                  │  │                  │  │                  │
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
│  create_buffers: │  │ create_buffers:  │  │ create_buffers:  │
│    └─> NULL      │  │   ├─> Creates    │  │    └─> NULL      │
│                  │  │   │   weights_buf│  │                  │
│  destroy_buffers:│  │ destroy_buffers: │  │ destroy_buffers: │
│    └─> NULL      │  │   ├─> Releases   │  │    └─> NULL      │
│                  │  │   │   weights_buf│  │                  │
│  set_kernel_args:│  │ set_kernel_args: │  │ set_kernel_args: │
│    ├─> Sets 4    │  │   ├─> Sets 5     │  │    ├─> Custom    │
│    │   arguments │  │   │   arguments  │  │    │   arguments │
│    │   (in, out, │  │   │   (in, out,  │  │    │             │
│    │    w, h)    │  │   │  weights,w,h)│  │                  │
└──────────────────┘  └──────────────────┘  └──────────────────┘
        │                     │                     │
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              │
                              ▼
              ┌───────────────────────────────┐
              │   AUTO-REGISTRY SYSTEM        │
              │  (src/utils/auto_registry.c)  │
              │   Generated by build script   │
              │                               │
              │  algorithms[32]               │
              │    [0] = &dilate3x3_algorithm │
              │    [1] = &gaussian5x5_algo    │
              │    [2] = &algorithm3          │
              │    [3] = NULL                 │
              │                               │
              │  ⚙️ Auto-detects custom       │
              │     buffer functions via      │
              │     generate_registry.sh      │
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
├── c_ref/                           ← CPU reference implementation
│   └── dilate3x3_ref.c
│       ├── dilate3x3_ref()          → Ground truth implementation
│       ├── dilate3x3_verify()       → Exact match verification
│       └── dilate3x3_set_kernel_args() → Sets kernel arguments
│
└── cl/                              ← GPU kernel variants
    ├── dilate0.cl                   → Basic version
    │   └── __kernel dilate3x3()       • Simple global memory
    │                                  • 9 reads per pixel
    │                                  • Baseline performance
    │
    └── dilate1.cl                   → Optimized version
        └── __kernel dilate3x3()       • Local memory tiling (16x16)
                                       • Halo loading strategy
                                       • 8x faster than basic

src/gaussian/                        ← Algorithm with custom buffers
│
├── c_ref/
│   └── gaussian5x5_ref.c
│       ├── gaussian5x5_ref()        → Reference implementation
│       ├── gaussian5x5_verify()     → Tolerance-based verification
│       ├── gaussian5x5_create_buffers()   → Creates weights buffer
│       ├── gaussian5x5_destroy_buffers()  → Releases weights buffer
│       └── gaussian5x5_set_kernel_args()  → Sets 5 kernel arguments
│
└── cl/
    └── gaussian0.cl                 → Kernel using weights buffer
        └── __kernel gaussian5x5()     • Reads weights from buffer
                                       • Reduces register pressure

Auto-Registration (Generated):
──────────────────────────────
src/utils/auto_registry.c            ← Auto-generated by build system
  • Scans all *_ref.c files
  • Detects custom buffer functions
  • Generates Algorithm structures
  • Auto-registers before main()

Configuration (Per-Algorithm):
──────────────────────────────
config/dilate3x3.ini                 ← Per-algorithm configuration
  op_id = dilate3x3
  [kernel_0]
    kernel_file = dilate/cl/dilate0.cl
    kernel_name = dilate3x3
    global_work_size = 1920,1080
    local_work_size = 16,16

config/gaussian5x5.ini
  op_id = gaussian5x5
  [kernel_0]
    kernel_file = gaussian/cl/gaussian0.cl
    kernel_name = gaussian5x5
    global_work_size = 1920,1080
    local_work_size = 16,16

Test Data Organization:
───────────────────────
test_data/
├── input.bin                        ← Test image
├── dilate3x3/
│   ├── golden/
│   │   └── dilate3x3.bin            ← Cached C reference output
│   └── kernels/
│       ├── dilate0.bin              ← Cached compiled kernel v0
│       └── dilate1.bin              ← Cached compiled kernel v1
└── gaussian5x5/
    ├── golden/
    │   └── gaussian5x5.bin
    └── kernels/
        └── gaussian0.bin

Adding a New Algorithm:
───────────────────────
1. Create src/myalgo/c_ref/myalgo_ref.c with:
   - myalgo_ref(const OpParams*)
   - myalgo_verify(const OpParams*, float*)
   - myalgo_set_kernel_args(...)
   - Optional: myalgo_create_buffers(...) and myalgo_destroy_buffers(...)

2. Create src/myalgo/cl/myalgo0.cl with kernel implementation

3. Create config/myalgo.ini with:
   op_id = myalgo
   [kernel_0]
   kernel_file = myalgo/cl/myalgo0.cl
   ...

4. Build - auto_registry.c is regenerated automatically!
```

---

## 6. Custom Buffer Management Architecture

```
┌───────────────────────────────────────────────────────────────────┐
│         CUSTOM BUFFER PATTERN (Example: Gaussian 5x5)             │
│                                                                   │
│  Use Case: Algorithm needs additional GPU buffers beyond         │
│            standard input/output (e.g., weights, LUTs, temp      │
│            buffers)                                               │
└───────────────────────────────────────────────────────────────────┘

1. Define Buffer Structure (gaussian5x5_ref.c):
   ──────────────────────────────────────────────
   typedef struct {
       cl_mem weights_buf;      /* 5x5 Gaussian kernel weights */
   } Gaussian5x5Buffers;


2. Implement CreateBuffersFunc:
   ────────────────────────────
   void* gaussian5x5_create_buffers(cl_context context,
                                    const OpParams* params,
                                    const unsigned char* input,
                                    size_t img_size) {
       Gaussian5x5Buffers* buffers = malloc(...);

       /* Create read-only buffer for kernel weights */
       buffers->weights_buf = clCreateBuffer(
           context,
           CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
           sizeof(kernel_weights),
           kernel_weights,
           &err);

       return buffers;  /* Opaque pointer */
   }


3. Implement DestroyBuffersFunc:
   ─────────────────────────────
   void gaussian5x5_destroy_buffers(void* algo_buffers) {
       Gaussian5x5Buffers* buffers = (Gaussian5x5Buffers*)algo_buffers;

       clReleaseMemObject(buffers->weights_buf);
       free(buffers);
   }


4. Update SetKernelArgsFunc:
   ─────────────────────────
   int gaussian5x5_set_kernel_args(cl_kernel kernel,
                                   cl_mem input_buf,
                                   cl_mem output_buf,
                                   const OpParams* params,
                                   void* algo_buffers) {
       Gaussian5x5Buffers* buffers = (Gaussian5x5Buffers*)algo_buffers;

       clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
       clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
       clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffers->weights_buf); // Custom!
       clSetKernelArg(kernel, 3, sizeof(int), &params->src_width);
       clSetKernelArg(kernel, 4, sizeof(int), &params->src_height);

       return 0;
   }


5. Update OpenCL Kernel (gaussian0.cl):
   ────────────────────────────────────
   __kernel void gaussian5x5(__global const uchar* input,
                             __global uchar* output,
                             __global const float* weights,  // ← Custom buffer
                             int width,
                             int height) {
       float weight = weights[idx];  // Read from buffer
       ...
   }


6. Auto-Registration (scripts/generate_registry.sh):
   ──────────────────────────────────────────────────
   • Build script automatically detects:
     - gaussian5x5_create_buffers() function exists
     - gaussian5x5_destroy_buffers() function exists

   • Generates registration with custom functions:
     static Algorithm gaussian5x5_algorithm = {
         ...
         .create_buffers = gaussian5x5_create_buffers,
         .destroy_buffers = gaussian5x5_destroy_buffers,
         ...
     };


LIFECYCLE:
──────────
  main.c calls:

  algo_buffers = algo->create_buffers(context, params, input, size);
                           ↓
  algo->set_kernel_args(kernel, in_buf, out_buf, params, algo_buffers);
                           ↓
  clEnqueueNDRangeKernel(...)  // Execute kernel
                           ↓
  algo->destroy_buffers(algo_buffers);  // Cleanup


BENEFITS:
─────────
  ✓ Encapsulation - Each algorithm manages its own buffers
  ✓ Flexibility - Supports complex algorithms with custom needs
  ✓ Type Safety - Opaque pointer prevents misuse
  ✓ Automatic - Build system auto-detects and registers
  ✓ Optional - Algorithms without custom buffers just use NULL
```

---

## 7. Data Flow Through the System

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
│                   Per-Algorithm Config Files                  │
│                                                               │
│  config/dilate3x3.ini        config/gaussian5x5.ini          │
│  ─────────────────────       ──────────────────────          │
│  op_id = dilate3x3           op_id = gaussian5x5             │
│  src_width = 1920            src_width = 1920                │
│  src_height = 1080           src_height = 1080               │
│                                                               │
│  [kernel_0]                  [kernel_0]                      │
│  kernel_file = dilate/       kernel_file = gaussian/         │
│      cl/dilate0.cl              cl/gaussian0.cl              │
│  kernel_name = dilate3x3     kernel_name = gaussian5x5       │
│  global_work_size=1920,1080  global_work_size=1920,1080      │
│  local_work_size = 16,16     local_work_size = 16,16         │
│                                                               │
│  [kernel_1]                  [kernel_1] (future)             │
│  kernel_file = dilate/       (optimizations)                 │
│      cl/dilate1.cl                                           │
│  ...                         ...                             │
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
3. **Auto-Discovery** - Build system automatically detects and registers algorithms
4. **Custom Buffers** - Flexible support for algorithm-specific GPU memory
5. **Maintainability** - Clear separation of concerns
6. **Performance** - Caching system accelerates iteration
7. **Safety** - MISRA-C compliance for production use (with exceptions for malloc in custom buffers)
8. **Flexibility** - Per-algorithm INI configuration enables runtime tuning
9. **Verification** - Dual implementation ensures correctness
10. **Scalability** - Supports multiple algorithms with multiple variants each

### Key Architectural Patterns

**Plugin Architecture with Auto-Registry**
- Algorithms discovered at build time by scanning `*_ref.c` files
- Auto-generated `auto_registry.c` eliminates manual registration
- Custom buffer functions automatically detected via grep

**Flexible Parameter System (OpParams)**
- Single unified interface supporting diverse algorithm needs
- Extensible with `algo_params` for custom parameters
- Supports different input/output dimensions for resize/scale operations

**Custom Buffer Management**
- Optional `CreateBuffersFunc`/`DestroyBuffersFunc` for advanced algorithms
- Opaque pointer pattern ensures type safety
- Examples: Gaussian weights buffer, LUTs, temporary buffers

**Per-Algorithm Configuration**
- Each algorithm has its own config file (e.g., `gaussian5x5.ini`)
- Multiple kernel variants per algorithm
- Independent tuning without conflicts

The design follows professional software engineering practices suitable for client demonstrations and production deployment.
