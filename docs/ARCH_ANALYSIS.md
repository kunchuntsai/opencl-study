# Architecture Dependency Analysis

**Date**: 2025-12-06
**Status**: Initial analysis of proposed library structure

## Overview

This document analyzes the dependency structure of the new directory layout that separates the codebase into library components (core, platform, utils) and user-facing code (examples).

**Goal**: Enable core, platform, and utils to be wrapped as a library, with users only needing to include headers from `include/` when implementing new algorithms in `examples/`.

## Directory Structure

```
src/
├── core/          # Algorithm orchestration & registry
│   ├── algorithm_runner.c
│   ├── op_registry.c
│   └── auto_registry.c
├── platform/      # OpenCL abstraction layer
│   ├── opencl_utils.c
│   ├── cache_manager.c
│   ├── cl_extension_api.c
│   └── *.h (internal headers)
├── utils/         # Generic utilities
│   ├── image_io.c
│   ├── verify.c
│   ├── config.c
│   └── *.h (internal headers)
└── main.c         # Entry point

include/           # Public API headers
├── op_interface.h
├── op_registry.h
└── algorithm_runner.h

examples/          # Algorithm implementations
├── gaussian/c_ref/gaussian5x5_ref.c
└── dilate/c_ref/dilate3x3_ref.c
```

## Dependency Analysis

### 1. Layered Architecture (Bottom to Top)

#### Layer 1: utils/ (Foundation Layer)

**Purpose**: Generic utilities, completely standalone

**Files**:
- `image_io.c` - Image loading/saving
- `verify.c` - Result verification utilities
- `config.c` - INI configuration parsing
- `safe_ops.h` - Safe arithmetic operations

**Dependencies**: Only standard C libraries

**Headers**: Internal headers only (`image_io.h`, `verify.h`, `config.h`, `safe_ops.h`)

**Status**: ✅ **Clean** - No upward dependencies

---

#### Layer 2: platform/ (OpenCL Abstraction)

**Purpose**: OpenCL platform abstraction and resource management

**Files**:
- `opencl_utils.c` - OpenCL initialization, kernel compilation
- `cache_manager.c` - Binary cache and output management
- `cl_extension_api.c` - Custom OpenCL extensions

**Dependencies**:
- ⬇️ **utils layer**: `utils/config.h`, `utils/safe_ops.h`
- ⬆️ **include/**: `op_interface.h` (for OpenCL types and buffer definitions)

**Cross-dependencies within platform**:
- `opencl_utils.c` → `cache_manager.h`
- `opencl_utils.c` → `cl_extension_api.h`

**Status**: ⚠️ **Issue** - References `op_interface.h` from include/, creating circular dependency

---

#### Layer 3: core/ (Orchestration Layer)

**Purpose**: Algorithm execution orchestration and registry management

**Files**:
- `algorithm_runner.c` - Main execution pipeline
- `op_registry.c` - Algorithm registry
- `auto_registry.c` - Auto-generated registration

**Dependencies**:
- ⬇️ **platform layer**: `platform/cache_manager.h`
- ⬇️ **utils layer**: `utils/image_io.h`, `utils/verify.h`, `utils/safe_ops.h`, `utils/config.h`
- ⬆️ **include/**: `algorithm_runner.h`, `op_registry.h`, `op_interface.h`

**Status**: ✅ **Good** - Proper layering, depends on lower layers only

---

#### Layer 4: examples/ (User Algorithm Layer)

**Purpose**: User-implemented algorithms (dilate, gaussian, etc.)

**Files**: `examples/*/c_ref/*.c`

**Current Dependencies**:
- ⬆️ **include/**: `op_interface.h`, `op_registry.h`
- ⬇️ **utils** (direct): `utils/safe_ops.h`, `utils/verify.h` ⚠️

**Status**: ⚠️ **Issue** - Directly includes utils headers instead of going through include/

---

### 2. Public API Analysis (include/)

**Current Public Headers**:
1. `op_interface.h` - Core algorithm interface (types, structs, callbacks)
2. `op_registry.h` - Algorithm registration macros (REGISTER_ALGORITHM)
3. `algorithm_runner.h` - High-level execution pipeline

**Problematic Includes in Public Headers**:

`include/algorithm_runner.h` includes:
- ✅ `op_registry.h` (public API)
- ⚠️ `platform/opencl_utils.h` (internal header exposed)
- ⚠️ `utils/config.h` (internal header exposed)

**Problem**: Users including public headers are forced to have access to internal implementation headers.

---

## Dependency Issues Identified

### Issue #1: examples/ Bypasses include/ and Uses utils/ Directly

**Location**: `examples/*/c_ref/*.c`

```c
#include "op_interface.h"      // ✅ Public API
#include "op_registry.h"       // ✅ Public API
#include "utils/safe_ops.h"    // ❌ Direct internal dependency
#include "utils/verify.h"      // ❌ Direct internal dependency
```

**Problem**: Example algorithms should only depend on `include/`, but they directly access internal utils headers.

**Impact**: Users need to know about internal implementation details.

**Recommendation**:
- **Option A**: Re-export needed utils functions in `include/` headers
- **Option B**: Accept that utils is part of the public library API and document it
- **Option C**: Move commonly needed utils to `include/utils/` as part of public API

---

### Issue #2: platform/ Has Upward Dependency on include/

**Location**: `src/platform/opencl_utils.c:9`

```c
#include "op_interface.h"  // ❌ Upward dependency
```

**Problem**: Creates circular dependency pattern:
- `platform/` → `include/op_interface.h`
- `include/algorithm_runner.h` → `platform/opencl_utils.h`

**Root Cause**: `op_interface.h` defines both:
- User-facing algorithm interface
- Platform types (RuntimeBuffer, CustomBuffers with cl_mem)

**Recommendation**:
- **Option A**: Split `op_interface.h` into two files:
  - `include/algorithm_interface.h` (user-facing)
  - `platform/opencl_types.h` (internal OpenCL types)
- **Option B**: Accept bidirectional dependency and document it as platform types being part of public API

---

### Issue #3: Public Headers Expose Internal Dependencies

**Location**: `include/algorithm_runner.h`

```c
#include "platform/opencl_utils.h"  // ❌ Exposes internal platform header
#include "utils/config.h"           // ❌ Exposes internal utils header
```

**Problem**:
- Users must have access to internal headers to compile
- Breaks encapsulation
- Prevents clean library boundary

**Recommendation**: Use forward declarations and opaque pointers

**Example**:
```c
// Instead of:
#include "platform/opencl_utils.h"
void RunAlgorithm(..., OpenCLEnv* env, ...);

// Use:
typedef struct OpenCLEnv OpenCLEnv;  // Forward declaration
void RunAlgorithm(..., OpenCLEnv* env, ...);
```

---

## Ideal Dependency Graph

```
┌─────────────────┐
│   examples/     │  User Algorithm Implementations
│  (user code)    │  - Only includes from include/
└────────┬────────┘
         │ uses public API
         ▼
┌─────────────────┐
│    include/     │  Public API Headers
│  (public API)   │  - op_interface.h
└────────┬────────┘  - op_registry.h
         │           - algorithm_runner.h (no internal deps)
         ▼
┌─────────────────┐
│     core/       │  Algorithm Orchestration
│ (library impl)  │  - algorithm_runner.c
└────────┬────────┘  - op_registry.c
         │
         ▼
┌─────────────────┐
│   platform/     │  OpenCL Abstraction
│ (library impl)  │  - opencl_utils.c
└────────┬────────┘  - cache_manager.c
         │           - cl_extension_api.c
         ▼
┌─────────────────┐
│     utils/      │  Generic Utilities
│ (library impl)  │  - image_io.c
└─────────────────┘  - verify.c
                     - config.c
                     - safe_ops.h
```

**Key Principle**: Dependencies flow downward only, no circular references.

---

## Recommendations for Library Design

### Option A: Clean Separation (Recommended for True Library)

#### 1. Fix Public API Headers

**Make `include/` headers self-contained with no internal dependencies**:

```c
// include/algorithm_runner.h (cleaned up)
#pragma once

#include "op_registry.h"

// Forward declarations (no internal headers needed)
typedef struct OpenCLEnv OpenCLEnv;
typedef struct Config Config;
typedef struct KernelConfig KernelConfig;

void RunAlgorithm(const Algorithm* algo,
                  const KernelConfig* kernel_cfg,
                  const Config* config,
                  OpenCLEnv* env,
                  unsigned char* gpu_output_buffer,
                  unsigned char* ref_output_buffer);
```

#### 2. Split op_interface.h

**Separate user-facing and platform types**:

- `include/algorithm_interface.h` - User-facing types (Algorithm, OpParams)
- `platform/opencl_types.h` - Internal OpenCL buffer types
- Both include the necessary subset

#### 3. Handle Utils Dependency

**Choice A**: Re-export in include/
```c
// include/algorithm_utils.h
#pragma once
int VerifyResult(const OpParams* params, float* max_error);
```

**Choice B**: Make utils explicitly public
```
include/
├── op_interface.h
├── op_registry.h
├── algorithm_runner.h
└── utils/              # Public utilities
    ├── safe_ops.h
    └── verify.h
```

#### 4. CMake for Clean Library

```cmake
# Library target
add_library(opencl_imgproc STATIC
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UTILS_SOURCES}
)

target_include_directories(opencl_imgproc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include      # Only public API
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src          # Internal headers
)

# User code (examples) links against library
add_executable(opencl_host
    ${MAIN_SOURCES}
    ${ALGO_SOURCES}
)

target_link_libraries(opencl_host opencl_imgproc)
```

---

### Option B: Pragmatic Approach (Easier, Less Clean)

#### 1. Accept Multi-Layer Public API

Document that the library exposes three layers:
- `include/` - Primary API
- `src/utils/` - Utility functions (part of public API)
- `src/platform/` - Platform types (part of public API)

#### 2. CMake Setup

```cmake
include_directories(
    ${LIB_ROOT}/include
    ${LIB_ROOT}/src/utils      # Part of public API
    ${LIB_ROOT}/src/platform   # Part of public API
    ${LIB_ROOT}/src/core       # Part of public API
)
```

#### 3. Documentation

Create `include/README.md`:
```markdown
# Public API

To use this library, include these directories:
- include/ - Main API headers
- src/utils/ - Utility headers (safe_ops.h, verify.h)
- src/platform/ - Platform types (for advanced usage)

Include in your algorithm implementations:
#include "op_interface.h"     // Required
#include "op_registry.h"      // Required for REGISTER_ALGORITHM
#include "utils/safe_ops.h"   // Optional utilities
#include "utils/verify.h"     // Optional verification helpers
```

---

## Current CMakeLists.txt Analysis

**Current setup**:
```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/include    # ✅ Public API
    ${CMAKE_SOURCE_DIR}/src        # ✅ Allows src/utils, src/platform paths
)
```

**Why it works (but isn't ideal)**:
- ✅ Core/platform/utils can reference each other via `platform/`, `utils/` prefixes
- ✅ Examples can use `op_interface.h` directly
- ⚠️ Examples can ALSO use `utils/` headers (bypassing clean API boundary)
- ⚠️ No enforcement of public vs. private headers

---

## Summary Table

| Layer | Purpose | Dependencies | Status |
|-------|---------|--------------|--------|
| **utils/** | Generic utilities | stdlib only | ✅ Clean |
| **platform/** | OpenCL abstraction | utils + op_interface.h | ⚠️ Circular with include/ |
| **core/** | Algorithm orchestration | platform + utils | ✅ Clean layering |
| **include/** | Public API | (should be none) | ⚠️ Exposes internal headers |
| **examples/** | User algorithms | (should be include/ only) | ⚠️ Uses utils directly |

---

## Next Steps

1. **Decide on approach**: Option A (clean separation) vs Option B (pragmatic)

2. **If Option A (Recommended)**:
   - [ ] Split `op_interface.h` into user-facing and platform-specific parts
   - [ ] Remove internal includes from `include/algorithm_runner.h`
   - [ ] Use forward declarations in public headers
   - [ ] Create `include/utils/` for public utility headers
   - [ ] Update examples to only include from `include/`
   - [ ] Create separate library target in CMake

3. **If Option B**:
   - [ ] Document the multi-layer public API clearly
   - [ ] Create `include/README.md` explaining what to include
   - [ ] Accept current structure with clear documentation

4. **Testing**:
   - [ ] Verify examples compile with only `include/` in path
   - [ ] Test library can be built separately from examples
   - [ ] Validate no circular dependencies remain

---

## Conclusion

**The goal is achievable**, but requires addressing:
1. Circular dependency between `platform/` ↔ `include/`
2. Decision on whether `utils/` is part of public API
3. Cleaning up public headers to not expose internal dependencies

**Recommended path**: Option A for a truly reusable library that could be packaged and distributed.
