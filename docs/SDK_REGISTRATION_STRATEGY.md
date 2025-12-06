# SDK Registration Strategy

## Current Challenge

When building the library as a shared object (.so), we encounter:

```
Undefined symbols:
  "_Dilate3x3Ref", referenced from auto_registry.c
  "_Gaussian5x5Ref", referenced from auto_registry.c
```

**Why?** The library includes `auto_registry.c` which references algorithms from `examples/`, but examples are customer code (not part of the library).

## Solution: Two-Tier Registration Strategy

### Approach 1: Library Without Auto-Registry (Recommended for SDK)

**For SDK distribution**, the library should NOT include auto-registry. Customers provide their own registration.

#### Library Build (for SDK):
```cmake
# Build library WITHOUT auto_registry.c
add_library(opencl_imgproc SHARED
    ${CORE_SOURCES}           # algorithm_runner.c, op_registry.c
    ${PLATFORM_SOURCES}       # opencl_utils.c, cache_manager.c
    ${UTILS_SOURCES}          # config.c, image_io.c, verify.c
    # NOTE: NOT including auto_registry.c
)
```

#### Customer Application Build:
```cmake
# Customer builds their app with their algorithms
add_executable(customer_app
    main.c
    ${CUSTOMER_ALGO_SOURCES}   # Customer's algorithms
    auto_registry.c             # Generated from customer's algorithms
)

target_link_libraries(customer_app
    opencl_imgproc              # Your SDK library
)
```

**Workflow**:
1. Customer writes algorithms in their project
2. Customer runs `generate_registry.sh` in their project
3. Customer compiles `auto_registry.c` with their app
4. Customer links against `libopencl_imgproc.so`

### Approach 2: Remove Auto-Registry, Use Manual Registration

**Alternative**: Don't use auto-registry at all. Require explicit registration in main().

#### Customer's main.c:
```c
#include "op_interface.h"
#include "op_registry.h"

// Customer's algorithm implementations
extern Algorithm my_algo_algorithm;
extern Algorithm my_other_algo_algorithm;

int main() {
    // Manual registration
    RegisterAlgorithm(&my_algo_algorithm);
    RegisterAlgorithm(&my_other_algo_algorithm);

    // Rest of main logic...
}
```

#### Customer's algorithm file:
```c
#include "op_interface.h"
#include "op_registry.h"

void MyAlgoRef(const OpParams* params) { /* ... */ }
int MyAlgoVerify(const OpParams* params, float* max_error) { /* ... */ }
int MyAlgoSetKernelArgs(...) { /* ... */ }

// Declare algorithm struct (but don't auto-register)
Algorithm my_algo_algorithm = {
    .name = "My Algorithm",
    .id = "myalgo",
    .reference_impl = MyAlgoRef,
    .verify_result = MyAlgoVerify,
    .set_kernel_args = MyAlgoSetKernelArgs
};
```

### Approach 3: Keep Auto-Registry for Internal Development

**For internal development**, keep using auto-registry with examples bundled.

```cmake
# Development build (includes examples in library)
if(SDK_BUILD)
    # SDK build: library only, no auto-registry
    add_library(opencl_imgproc SHARED
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
else()
    # Development build: include auto-registry
    add_library(opencl_imgproc STATIC
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
        ${CMAKE_SOURCE_DIR}/src/core/auto_registry.c
    )
endif()
```

## Recommended SDK Structure

### Option A: Provide Registration Script to Customers

**SDK Package includes**:
```
sdk/
├── lib/
│   └── libopencl_imgproc.so        # Library WITHOUT auto_registry
├── include/
│   └── ...                         # Public headers
├── tools/
│   └── generate_registry.sh       # Script for customers to use
└── examples/
    ├── main.c.example
    └── sample_algorithms/
```

**Customer usage**:
```bash
# Customer's project
my_project/
├── algorithms/
│   ├── algo1/c_ref/algo1_ref.c
│   └── algo2/c_ref/algo2_ref.c
└── sdk/
    └── ...

# Customer generates their own registry
cd my_project
../sdk/tools/generate_registry.sh ./algorithms ./auto_registry.c

# Customer builds
gcc main.c auto_registry.c algorithms/*/*.c \
    -I./sdk/include -L./sdk/lib -lopencl_imgproc \
    -o my_app
```

### Option B: Manual Registration (Simpler for Customers)

**SDK Package**:
```
sdk/
├── lib/
│   └── libopencl_imgproc.so
└── include/
    ├── op_interface.h
    ├── op_registry.h       # Contains RegisterAlgorithm()
    └── ...
```

**Customer code** (no auto-registry needed):
```c
// customer_main.c
#include "op_registry.h"

// Forward declare customer algorithms
extern Algorithm edge_detect_algorithm;
extern Algorithm blur_algorithm;

int main() {
    // Explicitly register
    RegisterAlgorithm(&edge_detect_algorithm);
    RegisterAlgorithm(&blur_algorithm);

    // ... rest of main logic
}
```

**Simpler** but requires customers to update main.c when adding algorithms.

## Comparison

| Approach | Pros | Cons | Recommended For |
|----------|------|------|-----------------|
| **Option A: Customer Auto-Registry** | ✅ Automatic registration<br>✅ Clean separation | ❌ Requires script<br>❌ Extra build step | Production SDK |
| **Option B: Manual Registration** | ✅ Simple<br>✅ No script needed<br>✅ Explicit | ❌ Manual updates<br>❌ Forgetting to register | Simple SDK, few algorithms |
| **Option C: Internal Auto-Registry** | ✅ Automatic for dev | ❌ Couples library to examples | Internal development only |

## Implementation Plan for SDK

### Phase 1: Update CMakeLists.txt

```cmake
# Add option for SDK build
option(SDK_BUILD "Build library for SDK distribution (excludes auto_registry)" OFF)

if(SDK_BUILD)
    # SDK build: core + platform + utils ONLY
    set(LIB_SOURCES
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
    # Filter out auto_registry.c
    list(FILTER LIB_SOURCES EXCLUDE REGEX "auto_registry\\.c$")
else()
    # Development build: include auto_registry
    set(LIB_SOURCES
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${UTILS_SOURCES}
    )
endif()

add_library(opencl_imgproc ${LIB_TYPE} ${LIB_SOURCES})
```

### Phase 2: Update create_sdk.sh

```bash
# Build library for SDK (without auto_registry)
cmake -DSDK_BUILD=ON -DBUILD_SHARED_LIBS=ON ..
make

# Package library + headers + tools
# Include generate_registry.sh in SDK package
```

### Phase 3: Documentation

Update `include/README.md` with registration instructions for customers.

## Recommended Approach for Your SDK

**I recommend Option A: Customer Auto-Registry**

**Why?**
- ✅ Maintains the convenience of automatic registration
- ✅ Clean separation (library doesn't depend on customer code)
- ✅ Scalable (works with any number of algorithms)
- ✅ Consistent with current development workflow

**Customer Experience**:
```bash
# 1. Write algorithm
vim algorithms/myalgo/c_ref/myalgo_ref.c

# 2. Generate registry
./sdk/tools/generate_registry.sh algorithms auto_registry.c

# 3. Build (registry included in compilation)
gcc main.c auto_registry.c algorithms/*/*.c \
    -I./sdk/include -L./sdk/lib -lopencl_imgproc -o app

# 4. Run
./app config/myalgo.ini 0
```

**Simple, automatic, and clean!**
