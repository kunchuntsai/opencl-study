# Per-Algorithm Configuration System

## Overview

The framework uses per-algorithm JSON configuration files for better maintainability and scalability. Each algorithm has its own `.json` file containing algorithm-specific settings and kernel variants.

## Configuration File Structure

```
config/
├── inputs.json        # Global input image definitions
├── outputs.json       # Global output image definitions
├── dilate3x3.json     # Dilate 3x3 specific config
├── gaussian5x5.json   # Gaussian 5x5 specific config
└── template.json      # Template for new algorithms
```

Each algorithm configuration file contains these sections:
- `input` - Input image reference
- `output` - Output image reference
- `verification` - Verification settings (tolerance, error rate)
- `scalars` - Scalar parameter definitions (optional)
- `buffers` - Custom buffer definitions (optional)
- `kernels` - Kernel variant configurations

## Auto-Derivation of op_id

The `op_id` field is **automatically derived** from the config filename:

| Config File                | Auto-derived op_id | Registered Algorithm Required |
|----------------------------|--------------------|-----------------------------|
| `config/dilate3x3.json`    | `dilate3x3`        | Yes - must have `dilate3x3` algorithm |
| `config/gaussian5x5.json`  | `gaussian5x5`      | Yes - must have `gaussian5x5` algorithm |
| `config/my_algo.json`      | `my_algo`          | Yes - must have `my_algo` algorithm |

## Configuration File Format

### Basic Structure

```json
{
    "input": {
        "input_image_id": "image_1"
    },
    "output": {
        "output_image_id": "output_1"
    },
    "verification": {
        "tolerance": 0,
        "error_rate_threshold": 0,
        "golden_source": "c_ref"
    },
    "scalars": {
        "my_param": {
            "type": "int",
            "value": 5
        }
    },
    "buffers": {
        "tmp_buffer": {
            "type": "READ_WRITE",
            "size_bytes": 400000
        }
    },
    "kernels": {
        "v0": {
            "kernel_file": "examples/algo/cl/algo.cl",
            "kernel_function": "algo_kernel",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]}
            ]
        }
    }
}
```

### Input/Output Section

References images defined in `config/inputs.json` and `config/outputs.json`:

```json
"input": {
    "input_image_id": "image_1"
},
"output": {
    "output_image_id": "output_1"
}
```

### Verification Section

| Parameter | Type | Description | Example |
|-----------|------|-------------|---------|
| `tolerance` | float | Max per-pixel difference allowed | `0` (exact), `1` (±1) |
| `error_rate_threshold` | float | Max fraction of pixels that can exceed tolerance | `0.001` (0.1%) |
| `golden_source` | string | Source of golden sample: `c_ref` or `file` | `"c_ref"` |
| `golden_file` | string | Path to golden file (when `golden_source` is `file`) | `"test_data/algo/golden.bin"` |

### Scalars Section

Define scalar parameters passed to kernels:

```json
"scalars": {
    "sigma": {
        "type": "float",
        "value": 1.4
    },
    "kernel_radius": {
        "type": "int",
        "value": 2
    },
    "buffer_size": {
        "type": "size_t",
        "value": "1920 * 1080 * 4"
    }
}
```

Supported types: `int`, `float`, `size_t`

### Buffers Section

Define custom OpenCL buffers:

**Empty buffer** (allocated but not initialized):
```json
"tmp_buffer": {
    "type": "READ_WRITE",
    "size_bytes": 400000
}
```

**File-backed buffer** (loaded from file):
```json
"kernel_weights": {
    "type": "READ_ONLY",
    "data_type": "float",
    "num_elements": 25,
    "source_file": "test_data/algo/weights.bin"
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `type` | enum | `READ_ONLY`, `WRITE_ONLY`, or `READ_WRITE` |
| `size_bytes` | int/string | Buffer size (supports expressions like `"1920 * 1080"`) |
| `data_type` | string | Element type: `float`, `uchar`, `int`, `short` |
| `num_elements` | int | Number of elements (for file-backed buffers) |
| `source_file` | string | Path to binary data file |

### Kernels Section

Define kernel variants with section name `v<N>` where `<N>` is the variant number:

```json
"kernels": {
    "v0": {
        "host_type": "standard",
        "kernel_option": "",
        "kernel_file": "examples/algo/cl/algo.cl",
        "kernel_function": "algo_kernel",
        "work_dim": 2,
        "global_work_size": [1920, 1088],
        "local_work_size": [16, 16],
        "kernel_args": [...]
    }
}
```

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `kernel_file` | string | Yes | Path to OpenCL kernel source file |
| `kernel_function` | string | Yes | Kernel function name |
| `work_dim` | int | Yes | Work dimensions (1, 2, or 3) |
| `global_work_size` | array | Yes | Global work size per dimension |
| `local_work_size` | array | Yes | Local work group size per dimension |
| `host_type` | string | No | `standard` (default) or `cl_extension` |
| `kernel_option` | string | No | Compiler options (e.g., `-cl-fast-relaxed-math`) |
| `kernel_args` | array | Yes | Kernel argument definitions |

### Kernel Arguments (kernel_args)

The new format uses descriptive keys with arrays:

| Key | Format | Description | Example |
|-----|--------|-------------|---------|
| `i_buffer` | `["type", "name"]` | Input buffer | `{"i_buffer": ["uchar", "src"]}` |
| `o_buffer` | `["type", "name"]` | Output buffer | `{"o_buffer": ["uchar", "dst"]}` |
| `buffer` | `["type", "name", size]` | Custom buffer with size | `{"buffer": ["uchar", "tmp", 45000]}` |
| `param` | `["type", "name"]` | Scalar parameter | `{"param": ["int", "src_width"]}` |

**Supported data types:**
- Buffers: `uchar`, `float`, `int`, `short`
- Params: `int`, `float`, `size_t`

**Example kernel_args:**
```json
"kernel_args": [
    {"i_buffer": ["uchar", "src"]},
    {"o_buffer": ["uchar", "dst"]},
    {"buffer": ["uchar", "tmp_global", 400000]},
    {"buffer": ["float", "kernel_weights", 100]},
    {"param": ["int", "src_width"]},
    {"param": ["int", "src_height"]},
    {"param": ["float", "sigma"]}
]
```

## Complete Example: Gaussian 5x5

```json
{
    "input": {
        "input_image_id": "image_2"
    },
    "output": {
        "output_image_id": "output_2"
    },
    "verification": {
        "tolerance": 1,
        "error_rate_threshold": 0.001
    },
    "scalars": {
        "sigma": {
            "type": "float",
            "value": 1.4
        }
    },
    "buffers": {
        "tmp_global": {
            "type": "READ_WRITE",
            "size_bytes": 400000
        },
        "kernel_x": {
            "type": "READ_ONLY",
            "data_type": "float",
            "num_elements": 5,
            "source_file": "test_data/gaussian5x5/kernel_x.bin"
        }
    },
    "kernels": {
        "v0": {
            "host_type": "standard",
            "kernel_file": "examples/gaussian5x5/cl/gaussian0.cl",
            "kernel_function": "gaussian5x5",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]},
                {"buffer": ["float", "kernel_x", 20]},
                {"buffer": ["float", "kernel_y", 20]}
            ]
        },
        "v1": {
            "host_type": "cl_extension",
            "kernel_file": "examples/gaussian5x5/cl/gaussian1.cl",
            "kernel_function": "gaussian5x5_optimized",
            "work_dim": 2,
            "global_work_size": [1920, 1088],
            "local_work_size": [16, 16],
            "kernel_args": [
                {"i_buffer": ["uchar", "src"]},
                {"o_buffer": ["uchar", "dst"]},
                {"buffer": ["uchar", "tmp_global", 400000]},
                {"param": ["int", "src_width"]},
                {"param": ["int", "src_height"]},
                {"buffer": ["float", "kernel_x", 20]},
                {"buffer": ["float", "kernel_y", 20]}
            ]
        }
    }
}
```

## Host Type Explanation

### `standard` (Default)
- Uses standard OpenCL API (`clSetKernelArg`, `clEnqueueNDRangeKernel`)
- Portable across all OpenCL implementations
- Best for: Initial implementations, cross-platform compatibility

### `cl_extension`
- Uses custom CL extension API framework
- Provides vendor-specific optimizations
- Supports advanced features like zero-copy buffers
- Best for: Performance-optimized variants, vendor-specific tuning

## Configuration Best Practices

1. **One algorithm = One config file**: Name your config file after the algorithm (e.g., `gaussian5x5.json`)
2. **Use descriptive buffer names**: `tmp_global` instead of `buf1`
3. **Specify buffer sizes inline**: Use the third element in buffer arrays
4. **Start with `standard` host type**: Add `cl_extension` variants for optimization
5. **Match kernel signatures exactly**: The order in `kernel_args` must match your kernel function parameters

## Implementation Details

- **Config path resolution:** `ResolveConfigPath()` in `src/utils/config.c` resolves algorithm names to `config/<name>.json`
- **op_id extraction:** `ExtractOpIdFromPath()` derives the op_id from the config filename
- **Kernel args parsing:** `ParseKernelArgsJson()` parses the new format with buffer sizes

See also: [CONFIG_KERNEL_ARG.md](CONFIG_KERNEL_ARG.md) for detailed kernel argument documentation.
