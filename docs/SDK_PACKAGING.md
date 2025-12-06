# SDK Packaging Guide

**Question**: Can we package core, platform, utils as a library (.so) and only expose main.c + headers to customers?

**Answer**: ✅ **YES** - The current architecture is designed exactly for this!

## Current Architecture Analysis

### What We Have Now

```
📦 Source Distribution (Development)
├── include/              # Public API headers
│   ├── op_interface.h
│   ├── op_registry.h
│   ├── algorithm_runner.h
│   └── utils/
│       ├── safe_ops.h
│       └── verify.h
├── src/                  # Internal implementation
│   ├── core/            # ⚙️ Business logic
│   ├── platform/        # ⚙️ OpenCL abstraction
│   └── utils/           # ⚙️ Utilities
├── examples/            # User algorithms
└── main.c              # Application entry
```

### What Customers Get (SDK Distribution)

```
📦 SDK Package
├── lib/
│   └── libopencl_imgproc.so      # 🔒 Compiled library (core + platform + utils)
├── include/                       # ✅ Public API headers only
│   ├── op_interface.h
│   ├── op_registry.h
│   ├── algorithm_runner.h
│   └── utils/
│       ├── safe_ops.h
│       └── verify.h
├── examples/                      # 📝 Sample code
│   └── main.c                    # ✅ Reference main implementation
└── docs/                         # 📚 Documentation
    └── API_REFERENCE.md
```

## How Easy Is This To Achieve?

### ✅ Already 90% There!

Our CMakeLists.txt already creates the library:

```cmake
# This already exists!
add_library(opencl_imgproc STATIC    # Currently STATIC
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILS_SOURCES}
)

target_include_directories(opencl_imgproc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include      # ✅ Already PUBLIC
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src          # ✅ Already PRIVATE
)
```

### What Needs To Change?

**Minor modification**: Change `STATIC` to `SHARED`:

```cmake
add_library(opencl_imgproc SHARED    # Changed from STATIC to SHARED
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILS_SOURCES}
)
```

That's literally it! 🎉

## SDK Packaging Strategy

### Option 1: Binary SDK (Recommended for Commercial Distribution)

**What customers get**:
- ✅ Precompiled `.so` library (no source code for core/platform/utils)
- ✅ Public headers (`include/`)
- ✅ Sample `main.c` with comments
- ✅ Documentation

**What customers can do**:
- ✅ Implement their own algorithms in `examples/`
- ✅ Customize `main.c` for their application
- ✅ Link against your library

**What customers CANNOT see**:
- ❌ Core business logic implementation
- ❌ Platform optimization details
- ❌ Internal utilities

**Build process for customers**:
```bash
# Customer's build
gcc main.c examples/my_algo/my_algo_ref.c \
    -I./include \
    -L./lib -lopencl_imgproc \
    -framework OpenCL \
    -o my_app
```

### Option 2: Source SDK (Recommended for Open Source)

**What customers get**:
- ✅ Full source code
- ✅ CMakeLists.txt to build library
- ✅ Can modify and extend

**Build process**:
```bash
# Customer builds library themselves
mkdir build && cd build
cmake ..
make
```

## Detailed Packaging Steps

### Step 1: Modify CMakeLists.txt for Shared Library

```cmake
# Option to build shared or static
option(BUILD_SHARED_LIBS "Build shared library" ON)

if(BUILD_SHARED_LIBS)
    add_library(opencl_imgproc SHARED
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
else()
    add_library(opencl_imgproc STATIC
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
endif()

# Version information
set_target_properties(opencl_imgproc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules for SDK packaging
install(TARGETS opencl_imgproc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(FILES src/main.c
    DESTINATION examples
)
```

### Step 2: Create SDK Package Structure

```bash
# Build script for creating SDK
#!/bin/bash

VERSION=1.0.0
SDK_DIR=opencl-imgproc-sdk-${VERSION}

# Build the library
mkdir -p build && cd build
cmake -DBUILD_SHARED_LIBS=ON ..
make

# Create SDK directory structure
mkdir -p ${SDK_DIR}/{lib,include,examples,docs}

# Copy compiled library
cp lib/libopencl_imgproc.so ${SDK_DIR}/lib/

# Copy public headers
cp -r ../include/* ${SDK_DIR}/include/

# Copy example main.c with documentation
cp ../src/main.c ${SDK_DIR}/examples/main.c.example

# Copy sample algorithms
cp -r ../examples/* ${SDK_DIR}/examples/

# Copy documentation
cp ../docs/*.md ${SDK_DIR}/docs/

# Create tarball
tar -czf ${SDK_DIR}.tar.gz ${SDK_DIR}
```

## Architecture Compliance Check

### ✅ Does Current Architecture Support This?

Let's verify:

#### 1. **Is `include/` self-contained?**
✅ **YES** - No internal headers exposed

```c
// include/algorithm_runner.h uses forward declarations
typedef struct OpenCLEnv OpenCLEnv;      // ✅ No internal header needed
typedef struct Config Config;            // ✅ No internal header needed
```

#### 2. **Can customers implement algorithms with only `include/`?**
✅ **YES** - Algorithm interface is complete

```c
// Customer's algorithm (examples/my_algo/my_algo_ref.c)
#include "op_interface.h"     // ✅ Available in SDK
#include "op_registry.h"      // ✅ Available in SDK
#include "utils/verify.h"     // ✅ Available in SDK

void MyAlgoRef(const OpParams* params) { /* ... */ }
int MyAlgoVerify(const OpParams* params, float* max_error) { /* ... */ }
int MyAlgoSetKernelArgs(cl_kernel k, cl_mem in, cl_mem out, const OpParams* p) { /* ... */ }

REGISTER_ALGORITHM(myalgo, "My Algorithm", MyAlgoRef, MyAlgoVerify, MyAlgoSetKernelArgs)
```

#### 3. **Can customers use `main.c` without seeing internal implementation?**
✅ **YES** - main.c only needs:
- Public headers (`include/`)
- Link against library (`-lopencl_imgproc`)

#### 4. **Are all internal details hidden?**
✅ **YES** - The library encapsulates:
- Core orchestration logic
- OpenCL platform utilities
- Configuration parsing
- Cache management

## Customer Usage Example

### Customer's Project Structure

```
customer-app/
├── CMakeLists.txt
├── main.c                          # From SDK examples/
├── my_algorithms/
│   ├── edge_detect/
│   │   └── c_ref/
│   │       └── edge_detect_ref.c  # Customer's algorithm
│   └── blur/
│       └── c_ref/
│           └── blur_ref.c         # Customer's algorithm
└── sdk/                           # Your SDK
    ├── lib/
    │   └── libopencl_imgproc.so
    └── include/
        ├── op_interface.h
        ├── op_registry.h
        └── utils/
```

### Customer's CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(CustomerApp)

# Find your SDK
set(OPENCL_IMGPROC_SDK ${CMAKE_SOURCE_DIR}/sdk)

# Include SDK headers
include_directories(${OPENCL_IMGPROC_SDK}/include)

# Link SDK library
link_directories(${OPENCL_IMGPROC_SDK}/lib)

# Find customer algorithms
file(GLOB_RECURSE ALGO_SOURCES "my_algorithms/*/c_ref/*.c")

# Build customer's app
add_executable(my_app
    main.c
    ${ALGO_SOURCES}
)

target_link_libraries(my_app
    opencl_imgproc    # Your SDK library
    OpenCL
    m
)
```

### Customer builds and runs:

```bash
cd customer-app
mkdir build && cd build
cmake ..
make

./my_app config/edge_detect.ini 0
```

## API Stability Considerations

### Public API (Must Remain Stable)

**These headers are the contract**:
- ✅ `include/op_interface.h` - Core types (Algorithm, OpParams, etc.)
- ✅ `include/op_registry.h` - Registration macros
- ✅ `include/algorithm_runner.h` - Pipeline interface
- ✅ `include/utils/*.h` - Utility functions

**Versioning strategy**:
```
v1.0.0 → Initial release
v1.1.0 → Add new features (backward compatible)
v2.0.0 → Breaking changes (incompatible API changes)
```

### Internal Implementation (Can Change Freely)

**These are hidden in the .so**:
- ✅ `core/` - Can refactor without breaking customers
- ✅ `platform/` - Can optimize without breaking customers
- ✅ `utils/` - Can change implementation without breaking customers

## Symbol Visibility Control (Advanced)

### Option: Hide Internal Symbols

Add to CMakeLists.txt:

```cmake
# Hide all symbols by default, only expose public API
set_target_properties(opencl_imgproc PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Mark public API functions
# In your public headers:
#define OPENCL_IMGPROC_API __attribute__((visibility("default")))

// Example in algorithm_runner.h
OPENCL_IMGPROC_API void RunAlgorithm(...);
```

This prevents customers from accidentally depending on internal functions.

## Distribution Models

### Model 1: Commercial Binary SDK

**Package**:
```
opencl-imgproc-sdk-1.0.0/
├── lib/
│   ├── libopencl_imgproc.so       # Release build (optimized)
│   └── libopencl_imgproc.so.1.0.0 # Versioned
├── include/                        # Public API
├── examples/
│   ├── main.c.example
│   └── sample_algorithms/
└── docs/
    ├── API_REFERENCE.md
    ├── GETTING_STARTED.md
    └── MIGRATION_GUIDE.md
```

**Licensing**: Proprietary license for binary, SDK license for headers

### Model 2: Open Source SDK

**Package**: Same structure + full source code

**Licensing**: MIT/Apache/GPL

### Model 3: Dual Licensing

- Free binary SDK for evaluation
- Commercial license for production
- Open source for academic/research

## Migration Path from Current Code

### Immediate (5 minutes):

```bash
# 1. Change STATIC to SHARED in CMakeLists.txt
sed -i '' 's/add_library(opencl_imgproc STATIC/add_library(opencl_imgproc SHARED/' CMakeLists.txt

# 2. Rebuild
./scripts/build.sh

# 3. Test
./build/opencl_host config/dilate3x3.ini 0
```

### Short-term (1-2 hours):

1. ✅ Add install rules to CMakeLists.txt
2. ✅ Create SDK packaging script
3. ✅ Write customer documentation
4. ✅ Add version information to library

### Medium-term (1 day):

1. ✅ Add symbol visibility controls
2. ✅ Create customer examples
3. ✅ Set up CI/CD for SDK packaging
4. ✅ Add SDK tests

## Comparison: Current vs SDK Package

| Aspect | Development (Current) | SDK Package |
|--------|----------------------|-------------|
| **Library Type** | Static (.a) | Shared (.so) |
| **Source Visibility** | Full source | Headers only |
| **Customer Sees** | Everything | Public API + main.c |
| **Customer Can** | Modify core | Implement algorithms |
| **Build Time** | Slower (rebuild all) | Faster (link only) |
| **Distribution Size** | ~50MB (source) | ~2MB (binary) |
| **IP Protection** | None | Core logic hidden |

## Conclusion

### Is This Architecture Easy to Package?

# ✅ **YES - EXTREMELY EASY!**

**Current architecture advantages**:

1. ✅ **Already separated PUBLIC/PRIVATE includes**
   - `include/` is self-contained
   - `src/` is isolated

2. ✅ **Already builds as a library**
   - Just change STATIC → SHARED

3. ✅ **Clean dependencies**
   - No circular dependencies
   - Clear layer boundaries

4. ✅ **Well-documented API**
   - `include/README.md` already exists
   - Customer-facing documentation ready

5. ✅ **Example usage pattern**
   - `examples/` shows how to use the API
   - `main.c` is a complete reference implementation

### Effort Required

| Task | Effort | Priority |
|------|--------|----------|
| Change to shared library | 5 minutes | ⭐⭐⭐ High |
| Add install rules | 30 minutes | ⭐⭐⭐ High |
| Create packaging script | 1 hour | ⭐⭐⭐ High |
| Write customer docs | 2 hours | ⭐⭐ Medium |
| Add versioning | 30 minutes | ⭐⭐ Medium |
| Symbol visibility | 1 hour | ⭐ Low |

**Total**: ~5 hours to production-ready SDK

### Recommendation

**You can create an SDK package TODAY** with minimal changes:

```bash
# Quick SDK creation (5 minutes)
1. Edit CMakeLists.txt: STATIC → SHARED
2. Build: ./scripts/build.sh
3. Package:
   - lib/libopencl_imgproc.so
   - include/*
   - examples/main.c
   - docs/*.md
4. Ship it! 🚀
```

Your architecture is **SDK-ready by design**! 🎉
