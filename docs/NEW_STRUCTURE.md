  Customer-Friendly Structure

  opencl-study/
  ├── README.md                    # "What is this and how do I use it?"
  │
  ├── include/                     # Public API (what customers import)
  │   ├── algorithm_runner.h       # Main execution API
  │   ├── op_interface.h          # Algorithm interface (customers implement this)
  │   └── op_registry.h           # Registry API
  │
  ├── src/
  │   ├── main.c                  # Example usage
  │   │
  │   ├── core/                   # Framework internals (don't touch)
  │   │   ├── algorithm_runner.c
  │   │   ├── op_registry.c
  │   │   └── auto_registry.c    # Generated
  │   │
  │   ├── platform/               # OpenCL platform code (don't touch)
  │   │   ├── opencl_utils.h
  │   │   ├── opencl_utils.c
  │   │   ├── cl_extension_api.h
  │   │   ├── cl_extension_api.c
  │   │   ├── cache_manager.h
  │   │   └── cache_manager.c
  │   │
  │   ├── support/                # Utilities (use as needed)
  │   │   ├── image_io.h
  │   │   ├── image_io.c
  │   │   ├── config.h
  │   │   ├── config.c
  │   │   ├── verify.h
  │   │   ├── verify.c
  │   │   └── safe_ops.h
  │   │
  │   └── Makefile
  │
  ├── examples/                   # ← CUSTOMER FOCUS HERE
  │   ├── README.md              # "How to add your algorithm"
  │   │
  │   ├── dilate3x3/
  │   │   ├── dilate3x3.c        # Reference implementation
  │   │   └── kernels/
  │   │       ├── v0_naive.cl
  │   │       └── v1_optimized.cl
  │   │
  │   └── gaussian5x5/
  │       ├── gaussian5x5.c
  │       └── kernels/
  │           ├── v0_naive.cl
  │           ├── v1_local_mem.cl
  │           └── v2_optimized.cl
  │
  ├── config/
  │   └── config.ini              # Easy configuration
  │
  ├── scripts/
  │   ├── new_algorithm.sh        # Helper: creates algorithm template
  │   └── generate_registry.sh    # Internal: auto-discovery
  │
  └── docs/
      ├── API.md                  # API reference
      ├── getting-started.md      # Quick start guide
      └── adding-algorithms.md    # Step-by-step tutorial

  Why This Works for Customers

  1. Clear "Touch Points"

  include/           → Use these headers in your algorithms
  examples/          → YOUR ALGORITHMS GO HERE
  docs/              → Read when you need help
  config/config.ini  → Configure behavior

  src/core/          → Framework internals (don't touch)
  src/platform/      → OpenCL infrastructure (don't touch)
  src/support/       → Utilities (use if needed)

  2. Professional Organization

  - include/ - Standard C/C++ convention (signals "this is the API")
  - examples/ - Clear that these are templates/references
  - docs/ - Professional documentation
  - Separation of concerns is obvious

  3. Trust Through Structure

  Customers see:
  include/           ✓ Clean public API
  src/core/          ✓ Organized internals
  examples/          ✓ Working examples
  docs/              ✓ Documentation
  tests/             ✓ Quality assurance (if you add)

  This signals: "This is production-quality code I can trust"

  4. Easy Customization

  # Customer workflow:
  cd examples/
  cp -r dilate3x3 my_custom_algo
  cd my_custom_algo
  # Edit my_custom_algo.c
  # Edit kernels/*.cl
  make

  5. Documentation-Driven

  examples/README.md:
  # Algorithm Examples

  This directory contains example algorithms showing how to extend the framework.

  ## Quick Start

  1. Copy an example algorithm:
     ```bash
     cp -r dilate3x3 my_algorithm

  2. Implement three functions in my_algorithm/my_algorithm.c:
    - MyAlgorithmRef() - CPU reference implementation
    - MyAlgorithmVerify() - Verification logic
    - MyAlgorithmSetKernelArgs() - OpenCL kernel setup
  3. Write your OpenCL kernels in my_algorithm/kernels/*.cl
  4. Register using the macro:
  REGISTER_ALGORITHM(my_algorithm, "My Algorithm",
                     MyAlgorithmRef, MyAlgorithmVerify,
                     MyAlgorithmSetKernelArgs)
  5. Rebuild and run:
  make
  # Edit config/config.ini: op_id = my_algorithm
  ./opencl_host

  Directory Structure

  Each algorithm is self-contained:
  my_algorithm/
  ├── my_algorithm.c          # Implementation + registration
  └── kernels/
      ├── v0_baseline.cl      # Initial version
      └── v1_optimized.cl     # Optimized version

  API Reference

  See docs/API.md for detailed API documentation.
  See include/op_interface.h for the algorithm interface contract.

  ### 6. **Clear Include Paths**

  Customer code is clean:
  ```c
  #include "op_interface.h"      // From include/
  #include "op_registry.h"       // From include/
  #include "support/verify.h"    // From src/support/ (optional)

  Makefile handles it:
  CFLAGS = -Iinclude -Isrc

  Alternative: Even Cleaner for SDK

  If this is truly meant as an SDK/framework:

  opencl-framework/
  ├── include/
  │   └── opencl_framework/      # Namespaced headers
  │       ├── algorithm.h        # Core algorithm interface
  │       ├── runner.h           # Execution engine
  │       └── utils.h            # Optional utilities
  │
  ├── lib/                       # Could provide pre-built .a/.so
  │   └── libopencl_framework.a
  │
  ├── examples/
  │   ├── dilate3x3/
  │   ├── gaussian5x5/
  │   └── template/              # Empty template to copy
  │
  ├── src/                       # Framework implementation
  │   └── [internal structure]
  │
  └── docs/
      ├── getting-started.md
      ├── api-reference.md
      └── tutorial.md

  Customers don't even see src/ - they just use include/ and lib/.

  My Recommendation for Customers

  Go with the first structure because:

  1. Professional appearance: Matches industry standards (like SDL, OpenCV, etc.)
  2. Clear boundaries: include/ = public API, examples/ = customer code
  3. Trust factor: Shows organized, well-maintained codebase
  4. Easy to explain: "Include our headers, follow the examples, you're done"
  5. Room to grow: Can add tests/, benchmarks/, tools/ directories later

  Key changes from current:
  1. Create include/ with public headers
  2. Rename utils/ → organized into core/, platform/, support/
  3. Move algorithms to examples/
  4. Create examples/README.md with clear instructions
  5. Add scripts/new_algorithm.sh helper script
  6. Update includes and Makefile

  Would you like me to implement this customer-friendly reorganization? I'll make sure:
  - ✅ Headers are properly organized
  - ✅ Include paths are clean
  - ✅ Examples are clear and well-documented
  - ✅ Everything still builds correctly
  - ✅ README explains the structure clearly

-------------
  Industry Standard Patterns

  Pattern 1: Strict Separation (Most "professional" looking)

  opencl-study/
  ├── include/                        # Public headers only
  │   └── opencl_framework/
  │       ├── algorithm_runner.h      # Public API
  │       ├── op_interface.h          # Algorithm contract
  │       └── op_registry.h           # Registry API
  │
  ├── src/
  │   ├── include/                    # Private headers
  │   │   ├── opencl_utils.h
  │   │   ├── cl_extension_api.h
  │   │   ├── cache_manager.h
  │   │   ├── image_io.h
  │   │   ├── config.h
  │   │   ├── verify.h
  │   │   └── safe_ops.h
  │   │
  │   ├── core/                       # Framework sources
  │   │   ├── algorithm_runner.c
  │   │   ├── op_registry.c
  │   │   └── auto_registry.c
  │   │
  │   ├── platform/                   # Platform sources
  │   │   ├── opencl_utils.c
  │   │   ├── cl_extension_api.c
  │   │   └── cache_manager.c
  │   │
  │   ├── support/                    # Support sources
  │   │   ├── image_io.c
  │   │   ├── config.c
  │   │   └── verify.c
  │   │
  │   └── main.c

  Pros:
  - ✅ Cleanest separation: public vs private
  - ✅ Very professional appearance
  - ✅ Common in libraries (libpng, zlib, etc.)
  - ✅ Clear where to find headers vs sources

  Cons:
  - ❌ Have to look in two places to understand a module
  - ❌ src/include/ can feel redundant

  Makefile:
  CFLAGS = -Iinclude -Isrc/include

  ---
  Pattern 2: Headers with Sources (Pragmatic)

  opencl-study/
  ├── include/                        # Public headers only
  │   └── opencl_framework/
  │       ├── algorithm_runner.h
  │       ├── op_interface.h
  │       └── op_registry.h
  │
  ├── src/
  │   ├── core/
  │   │   ├── algorithm_runner.c
  │   │   ├── op_registry.c
  │   │   └── auto_registry.c
  │   │
  │   ├── platform/
  │   │   ├── opencl_utils.h         # Private header, paired with .c
  │   │   ├── opencl_utils.c
  │   │   ├── cl_extension_api.h
  │   │   ├── cl_extension_api.c
  │   │   ├── cache_manager.h
  │   │   └── cache_manager.c
  │   │
  │   ├── support/
  │   │   ├── image_io.h
  │   │   ├── image_io.c
  │   │   ├── config.h
  │   │   ├── config.c
  │   │   ├── verify.h
  │   │   ├── verify.c
  │   │   └── safe_ops.h             # Header-only
  │   │
  │   └── main.c

  Pros:
  - ✅ Related files stay together
  - ✅ Easy to understand module boundaries
  - ✅ Used by major projects (nginx, Redis, FFmpeg modules)
  - ✅ Easier refactoring

  Cons:
  - ❌ Might look less formal at first glance
  - ❌ Need to add module dirs to include path

  Makefile:
  CFLAGS = -Iinclude -Isrc/platform -Isrc/support

  ---
  Pattern 3: Hybrid (Best for Your Use Case)

  opencl-study/
  ├── include/                        # Public API only
  │   └── opencl_framework/
  │       ├── algorithm_runner.h
  │       ├── op_interface.h
  │       └── op_registry.h
  │
  ├── src/
  │   ├── internal/                   # Shared private headers
  │   │   ├── opencl_utils.h
  │   │   ├── cache_manager.h
  │   │   ├── config.h
  │   │   └── safe_ops.h
  │   │
  │   ├── core/                       # Framework implementation
  │   │   ├── algorithm_runner.c
  │   │   ├── op_registry.c
  │   │   └── auto_registry.c
  │   │
  │   ├── platform/                   # Platform implementation
  │   │   ├── opencl_utils.c
  │   │   ├── cl_extension_api.h     # Only used by cl_extension_api.c
  │   │   ├── cl_extension_api.c
  │   │   └── cache_manager.c
  │   │
  │   ├── support/                    # Support implementation
  │   │   ├── image_io.h             # Only used by image_io.c
  │   │   ├── image_io.c
  │   │   ├── config.c
  │   │   ├── verify.h               # Only used by verify.c
  │   │   └── verify.c
  │   │
  │   └── main.c

  Logic:
  - Public headers → include/
  - Shared internal headers (used across modules) → src/internal/
  - Module-private headers (only used by one .c file) → stay with the .c

  Pros:
  - ✅ Clear API boundary (include/)
  - ✅ Clear what's shared vs module-private
  - ✅ Professional and practical
  - ✅ Scales well

  Cons:
  - ➖ Slightly more complex, but most logical

  Makefile:
  CFLAGS = -Iinclude -Isrc/internal -Isrc/support -Isrc/platform

  ---
  Real-World Examples

  OpenCV (Pattern 1 - Strict):
  include/opencv2/         # Public headers
  modules/core/src/        # All .cpp, no headers
  modules/core/src/        # Private headers in subdirs when needed

  nginx (Pattern 2 - Pragmatic):
  src/core/
      nginx.h
      nginx.c
      ngx_config.h
      ngx_config.c
  src/http/
      ngx_http.h
      ngx_http.c

  Linux Kernel (Pattern 2):
  include/                 # Public/exported headers
  kernel/
      sched/
          sched.h          # Private to sched module
          core.c
          fair.c

  ---
  My Recommendation for Your Project

  Use Pattern 3 (Hybrid) because:

  1. Clear API: include/ shows exactly what customers should use
  2. Logical separation: Shared internals in src/internal/, module-specific headers with their .c files
  3. Professional: Matches patterns from major projects
  4. Flexible: Easy to understand and maintain

  Specific structure:

  include/
  └── opencl_framework/
      ├── algorithm_runner.h     # Main API
      ├── op_interface.h         # Algorithm interface (customers implement)
      └── op_registry.h          # Registry API

  src/
  ├── internal/                  # Shared private headers
  │   ├── opencl_utils.h        # Used by multiple modules
  │   ├── cache_manager.h
  │   ├── config.h
  │   └── safe_ops.h
  │
  ├── core/
  │   ├── algorithm_runner.c    # Implements include/algorithm_runner.h
  │   ├── op_registry.c         # Implements include/op_registry.h
  │   └── auto_registry.c
  │
  ├── platform/
  │   ├── opencl_utils.c        # Uses internal/opencl_utils.h
  │   ├── cl_extension_api.h    # Only used here
  │   ├── cl_extension_api.c
  │   └── cache_manager.c
  │
  └── support/
      ├── image_io.h            # Only used by image_io.c
      ├── image_io.c
      ├── config.c              # Uses internal/config.h
      ├── verify.h              # Only used by verify.c
      └── verify.c

  This is professional, clear, and practical. Would you like me to implement this structure?