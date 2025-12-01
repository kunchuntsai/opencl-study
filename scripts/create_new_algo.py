#!/usr/bin/env python3
"""
Script to create a new algorithm template for the OpenCL framework.

Usage:
    python3 scripts/create_new_algo.py <algorithm_name>

Example:
    python3 scripts/create_new_algo.py resize

This will create:
- src/resize/c_ref/resize_ref.c (C reference implementation)
- src/resize/cl/resize0.cl (OpenCL kernel)
- config/resize.ini (Configuration file)
"""

import os
import sys
from pathlib import Path


def get_c_ref_template(algo_name, algo_name_upper):
    """Generate C reference implementation template."""
    return f'''#include "../../utils/safe_ops.h"
#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/verify.h"
#include <stddef.h>
#include <stdio.h>

/**
 * @brief {algo_name_upper} reference implementation
 *
 * CPU implementation of {algo_name} algorithm.
 * This serves as the ground truth for verifying GPU output.
 *
 * @param[in] params Operation parameters containing:
 *   - input: Input image buffer
 *   - output: Output image buffer
 *   - src_width, src_height: Source dimensions
 *   - dst_width, dst_height: Destination dimensions
 *   - custom_buffers: Optional custom buffers (NULL if none)
 */
void {algo_name}_ref(const OpParams* params) {{
    int y;
    int x;
    int width;
    int height;
    unsigned char* input;
    unsigned char* output;
    int total_pixels;

    if (params == NULL) {{
        return;
    }}

    /* Extract parameters */
    input = params->input;
    output = params->output;
    width = params->src_width;
    height = params->src_height;

    if ((input == NULL) || (output == NULL) || (width <= 0) || (height <= 0)) {{
        return;
    }}

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!safe_mul_int(width, height, &total_pixels)) {{
        return; /* Overflow detected */
    }}

    /* TODO: Implement your algorithm here */
    /* Example: Simple copy operation */
    for (y = 0; y < height; y++) {{
        for (x = 0; x < width; x++) {{
            int index = y * width + x;
            if (index < total_pixels) {{
                output[index] = input[index];
            }}
        }}
    }}
}}

/**
 * @brief Verify GPU result against reference
 *
 * Compares GPU output with reference implementation output.
 *
 * @param[in] params Operation parameters containing gpu_output and ref_output
 * @param[out] max_error Maximum absolute difference found
 * @return 1 if verification passed, 0 if failed
 */
int {algo_name}_verify(const OpParams* params, float* max_error) {{
    if (params == NULL) {{
        return 0;
    }}

    /* For exact match verification (e.g., for morphological operations) */
    /* Uncomment this if you need exact matching:
    int result = verify_exact_match(params->gpu_output, params->ref_output,
                                    params->dst_width, params->dst_height, 0);
    if (max_error != NULL) {{
        *max_error = (result == 1) ? 0.0f : 1.0f;
    }}
    return result;
    */

    /* For floating-point algorithms with tolerance */
    /* Allow small differences due to rounding (adjust tolerance as needed) */
    return verify_with_tolerance(params->gpu_output, params->ref_output,
                                params->dst_width, params->dst_height,
                                1.0f,      /* max_pixel_diff: 1 intensity level */
                                0.001f,    /* max_error_ratio: 0.1% of pixels can differ */
                                max_error);
}}

/**
 * @brief Set kernel arguments
 *
 * Sets all kernel arguments in the order expected by the OpenCL kernel.
 * Must match the kernel signature exactly.
 *
 * @param[in] kernel OpenCL kernel handle
 * @param[in] input_buf Input buffer
 * @param[in] output_buf Output buffer
 * @param[in] params Operation parameters
 * @return 0 on success, -1 on error
 */
int {algo_name}_set_kernel_args(cl_kernel kernel,
                                cl_mem input_buf,
                                cl_mem output_buf,
                                const OpParams* params) {{
    cl_uint arg_idx = 0U;

    if ((kernel == NULL) || (params == NULL)) {{
        return -1;
    }}

    /* Standard arguments: input, output, width, height */
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {{
        return -1;
    }}
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {{
        return -1;
    }}
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) != CL_SUCCESS) {{
        return -1;
    }}
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) != CL_SUCCESS) {{
        return -1;
    }}

    /* If your algorithm uses custom buffers, uncomment and modify this:
    if (params->custom_buffers == NULL) {{
        (void)fprintf(stderr, "Error: {algo_name_upper} requires custom buffers\\n");
        return -1;
    }}
    CustomBuffers* custom = params->custom_buffers;
    if (custom->count < 1) {{
        (void)fprintf(stderr, "Error: {algo_name_upper} requires at least 1 custom buffer\\n");
        return -1;
    }}

    // Set custom buffer arguments
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[0].buffer) != CL_SUCCESS) {{
        return -1;
    }}
    */

    return 0;
}}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * See src/utils/auto_registry.c for the registration code.
 */
'''


def get_opencl_kernel_template(algo_name, algo_name_upper):
    """Generate OpenCL kernel template."""
    return f'''/**
 * @file {algo_name}0.cl
 * @brief {algo_name_upper} OpenCL kernel implementation
 *
 * TODO: Add algorithm description here
 *
 * Kernel arguments:
 * @param input  Input image buffer
 * @param output Output image buffer
 * @param width  Image width in pixels
 * @param height Image height in pixels
 */

__kernel void {algo_name}(__global const uchar* input,
                         __global uchar* output,
                         int width,
                         int height) {{
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int index = y * width + x;

    /* TODO: Implement your algorithm here */
    /* Example: Simple copy operation */
    output[index] = input[index];
}}
'''


def get_config_template(algo_name, algo_name_upper):
    """Generate configuration file template."""
    return f'''# {algo_name_upper} Algorithm Configuration
# TODO: Add algorithm description here
# Note: op_id is auto-derived from filename ({algo_name}.ini -> op_id = {algo_name})

[image]
input = test_data/{algo_name}/input.bin
output = test_data/{algo_name}/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920
dst_height = 1080

# Variant 0: Basic implementation using standard OpenCL API
[kernel.v0]
host_type = standard   # Options: "standard" (default) or "cl_extension"
kernel_file = src/{algo_name}/cl/{algo_name}0.cl
kernel_function = {algo_name}
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

# Optional: Add custom buffers if needed
# Example: Custom buffer for algorithm-specific data
# [buffer.custom_data]
# type = READ_ONLY
# data_type = float
# num_elements = 100
# source_file = test_data/{algo_name}/custom_data.bin

# Optional: Add more kernel variants
# [kernel.v1]
# host_type = cl_extension
# kernel_file = src/{algo_name}/cl/{algo_name}1.cl
# kernel_function = {algo_name}_optimized
# work_dim = 2
# global_work_size = 1920,1088
# local_work_size = 16,16
'''


def create_algorithm_template(algo_name):
    """Create all template files for a new algorithm."""

    # Validate algorithm name
    if not algo_name.replace('_', '').replace('-', '').isalnum():
        print(f"Error: Algorithm name '{algo_name}' contains invalid characters.")
        print("Use only letters, numbers, underscores, and hyphens.")
        return 1

    # Normalize algorithm name (replace hyphens with underscores for C identifiers)
    algo_name_c = algo_name.replace('-', '_')
    algo_name_upper = algo_name_c.upper()

    # Get project root (assuming script is in scripts/ directory)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # Define paths
    src_dir = project_root / "src" / algo_name
    c_ref_dir = src_dir / "c_ref"
    cl_dir = src_dir / "cl"
    config_dir = project_root / "config"

    c_ref_file = c_ref_dir / f"{algo_name_c}_ref.c"
    cl_file = cl_dir / f"{algo_name_c}0.cl"
    config_file = config_dir / f"{algo_name}.ini"

    # Check if files already exist
    existing_files = []
    if c_ref_file.exists():
        existing_files.append(str(c_ref_file))
    if cl_file.exists():
        existing_files.append(str(cl_file))
    if config_file.exists():
        existing_files.append(str(config_file))

    if existing_files:
        print(f"Error: Algorithm '{algo_name}' already exists!")
        print("The following files already exist:")
        for f in existing_files:
            print(f"  - {f}")
        return 1

    # Create directories
    print(f"Creating algorithm template for: {algo_name}")
    print()

    c_ref_dir.mkdir(parents=True, exist_ok=True)
    print(f"✓ Created directory: {c_ref_dir.relative_to(project_root)}")

    cl_dir.mkdir(parents=True, exist_ok=True)
    print(f"✓ Created directory: {cl_dir.relative_to(project_root)}")

    config_dir.mkdir(parents=True, exist_ok=True)

    # Create C reference implementation
    with open(c_ref_file, 'w') as f:
        f.write(get_c_ref_template(algo_name_c, algo_name_upper))
    print(f"✓ Created file: {c_ref_file.relative_to(project_root)}")

    # Create OpenCL kernel
    with open(cl_file, 'w') as f:
        f.write(get_opencl_kernel_template(algo_name_c, algo_name_upper))
    print(f"✓ Created file: {cl_file.relative_to(project_root)}")

    # Create config file
    with open(config_file, 'w') as f:
        f.write(get_config_template(algo_name, algo_name_upper))
    print(f"✓ Created file: {config_file.relative_to(project_root)}")

    print()
    print("=" * 70)
    print("Algorithm template created successfully!")
    print("=" * 70)
    print()
    print("Next steps:")
    print(f"1. Implement the algorithm in: {c_ref_file.relative_to(project_root)}")
    print(f"2. Implement the OpenCL kernel in: {cl_file.relative_to(project_root)}")
    print(f"3. Configure parameters in: {config_file.relative_to(project_root)}")
    print(f"4. Create test data directory: test_data/{algo_name}/")
    print(f"5. Generate test input: python3 scripts/generate_test_image.py")
    print( "6. Rebuild the project: ./scripts/build.sh")
    print(f"7. Run your algorithm: ./build/opencl_host {algo_name} 0")
    print()
    print("See docs/ADD_NEW_ALGO.md for detailed implementation guide.")

    return 0


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 scripts/create_new_algo.py <algorithm_name>")
        print()
        print("Example:")
        print("  python3 scripts/create_new_algo.py resize")
        print("  python3 scripts/create_new_algo.py sobel_edge")
        print()
        return 1

    algo_name = sys.argv[1].lower()
    return create_algorithm_template(algo_name)


if __name__ == "__main__":
    sys.exit(main())
