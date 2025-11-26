# Per-Algorithm Configuration System

## Overview

The framework now uses per-algorithm configuration files for better maintainability and scalability. Each algorithm has its own `.ini` file containing algorithm-specific settings and kernel variants.

## Configuration File Structure

```
config/
├── dilate3x3.ini      # Dilate 3x3 specific config
├── gaussian5x5.ini    # Gaussian 5x5 specific config
├── myalgo.ini         # Your custom algorithm config
└── README.md          # Configuration documentation
```

### Config File Format

```ini
# Algorithm Configuration
# Note: op_id is auto-derived from filename (myalgo.ini -> op_id = myalgo)

[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920
dst_height = 1080

# Variant 0: Basic implementation
[kernel.v0]
kernel_file = src/myalgo/cl/myalgo0.cl
kernel_function = myalgo_kernel
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

# Variant 1: Optimized implementation
[kernel.v1]
kernel_file = src/myalgo/cl/myalgo1.cl
kernel_function = myalgo_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

## Usage Examples

### 1. By Algorithm Name (Recommended)
```bash
# Automatically loads config/<algorithm>.ini
./opencl_host dilate3x3 0          # Dilate, variant 0
./opencl_host gaussian5x5 0        # Gaussian, variant 0
./opencl_host myalgo 1             # Custom algo, variant 1
```

### 2. Interactive Variant Selection
```bash
# Select variant interactively after algorithm loads
./opencl_host dilate3x3
./opencl_host gaussian5x5
```

### 3. Explicit Config Path
```bash
# Use custom config file location
./opencl_host config/dilate3x3.ini 0
./opencl_host config/custom/test.ini 1
./opencl_host /path/to/custom.ini 0
```

### 4. Help
```bash
./opencl_host --help
./opencl_host -h
./opencl_host help
```

## Auto-Derivation of op_id

The `op_id` field is now **automatically derived** from the config filename:

| Config File            | Auto-derived op_id |
|------------------------|--------------------|
| `config/dilate3x3.ini` | `dilate3x3`        |
| `config/gaussian5x5.ini` | `gaussian5x5`   |
| `config/my_algo.ini`   | `my_algo`          |
| `custom/test.ini`      | `test`             |

You can still manually specify `op_id` in the `[image]` section if needed, but it's optional.

## Benefits

### ✅ Organization
- Each algorithm has its own config file
- Clear separation of concerns
- Easy to find and modify algorithm settings

### ✅ Scalability
- Adding new algorithms: just create `config/myalgo.ini`
- No need to edit a monolithic config file
- No conflicts when multiple developers work on different algorithms

### ✅ Version Control
- Clean git commits per algorithm
- No merge conflicts in shared config
- Each algorithm config can be versioned independently

### ✅ Maintenance
- Algorithm-specific variants kept together
- Easy to add/remove/modify variants
- Self-documenting (filename = algorithm name)

### ✅ Flexibility
- Support for algorithm-specific parameters (future)
- Can have different image sizes per algorithm
- Custom kernel work group sizes per algorithm

## Adding a New Algorithm

**Step 1:** Implement your algorithm
```c
// src/myalgo/c_ref/myalgo_ref.c
void myalgo_ref(const OpParams* params) {
    // Implementation
}

static int myalgo_verify(const OpParams* params, float* max_error) {
    // Verification
}

REGISTER_ALGORITHM(myalgo, "My Algorithm", myalgo_ref, myalgo_verify)
```

**Step 2:** Create config file `config/myalgo.ini`
```ini
[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920
dst_height = 1080

[kernel.v0]
kernel_file = src/myalgo/cl/myalgo.cl
kernel_function = myalgo_kernel
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

**Step 3:** Run it
```bash
./opencl_host myalgo 0
```

That's it! No modifications to existing code or configs needed.

## Configuration Best Practices

1. **One algorithm = One config file**: Name your config file after the algorithm (e.g., `dilate3x3.ini`)
2. **Omit op_id field**: The filename automatically becomes the op_id
3. **Use descriptive variant names**: Add comments explaining what each variant optimizes
4. **Group related variants**: Put all variants of an algorithm in its config file

## Command-Line Parsing Logic

The framework intelligently determines what you mean:

1. **Algorithm name** (e.g., `dilate3x3`) → `config/<name>.ini`
2. **Path with `/` or `.ini`** → Explicit config file path
3. **No variant index** → Interactive variant selection
4. **With variant index** → Direct execution with specified variant

This makes the CLI intuitive and flexible for all use cases.
