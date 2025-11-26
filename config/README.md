# Configuration Files

## Quick Start

Each algorithm has its own configuration file:

```bash
./opencl_host dilate3x3 0      # Run dilate with variant 0
./opencl_host gaussian5x5 0    # Run gaussian with variant 0
```

## Available Configurations

- **`dilate3x3.ini`** - Morphological dilation (3x3 structuring element)
  - v0: Basic implementation
  - v1: Optimized with local memory

- **`gaussian5x5.ini`** - Gaussian blur (5x5 kernel)
  - v0: Basic implementation

## Configuration File Format

```ini
# Comments start with #
# op_id is auto-derived from filename (myalgo.ini → op_id = myalgo)

[image]
input = test_data/input.bin
output = test_data/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920
dst_height = 1080

# Define multiple kernel variants
[kernel.v0]
kernel_file = src/myalgo/cl/myalgo0.cl
kernel_function = myalgo_kernel
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

[kernel.v1]
kernel_file = src/myalgo/cl/myalgo1.cl
kernel_function = myalgo_optimized
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16
```

## Adding a New Algorithm Config

1. Create `config/myalgorithm.ini`
2. Copy the template above
3. Update kernel paths and work group sizes
4. Run: `./opencl_host myalgorithm 0`

## Field Descriptions

### [image] Section

- `input` - Path to input image file (raw binary format)
- `output` - Path to output image file
- `src_width` / `src_height` - Input image dimensions
- `dst_width` / `dst_height` - Output image dimensions (for resize ops)

### [kernel.vN] Sections

- `kernel_file` - Path to OpenCL kernel source (.cl file)
- `kernel_function` - Name of kernel function to execute
- `work_dim` - Work dimensions (1, 2, or 3)
- `global_work_size` - Total work items (comma-separated for each dimension)
- `local_work_size` - Work group size (comma-separated)

## Notes

- **Filename = Algorithm ID**: `dilate3x3.ini` automatically sets `op_id = dilate3x3`
- **Multiple Variants**: Add `[kernel.v0]`, `[kernel.v1]`, etc. for different implementations
- **Work Group Sizes**: Must be compatible with your GPU (typically 16x16 for 2D)
- **Global Work Size**: Should cover entire image (may need padding for alignment)

For more details, see: `docs/CONFIG_SYSTEM.md`
