#!/bin/bash
################################################################################
# create_new_algo.sh
# Create new algorithm template for OpenCL framework
#
# Usage:
#   ./scripts/create_new_algo.sh <algorithm_name>
#
# Example:
#   ./scripts/create_new_algo.sh resize
#
# This will create:
# - src/<algo>/c_ref/<algo>_ref.c (C reference implementation)
# - src/<algo>/cl/<algo>0.cl (OpenCL kernel)
# - config/<algo>.ini (Configuration file)
################################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check arguments
if [ $# -ne 1 ]; then
    echo "Usage: $0 <algorithm_name>"
    echo ""
    echo "Example:"
    echo "  $0 resize"
    echo "  $0 sobel_edge"
    echo ""
    exit 1
fi

# Get algorithm name and convert to lowercase
ALGO_NAME=$(echo "$1" | tr '[:upper:]' '[:lower:]')

# Validate algorithm name (alphanumeric, underscore, hyphen only)
if ! echo "$ALGO_NAME" | grep -qE '^[a-z0-9_-]+$'; then
    echo -e "${RED}Error: Algorithm name '$ALGO_NAME' contains invalid characters.${NC}"
    echo "Use only letters, numbers, underscores, and hyphens."
    exit 1
fi

# Normalize name for C identifiers (replace hyphens with underscores)
ALGO_NAME_C=$(echo "$ALGO_NAME" | tr '-' '_')
ALGO_NAME_UPPER=$(echo "$ALGO_NAME_C" | tr '[:lower:]' '[:upper:]')

# Define paths
C_REF_DIR="src/${ALGO_NAME}/c_ref"
CL_DIR="src/${ALGO_NAME}/cl"
C_REF_FILE="${C_REF_DIR}/${ALGO_NAME_C}_ref.c"
CL_FILE="${CL_DIR}/${ALGO_NAME_C}0.cl"
CONFIG_FILE="config/${ALGO_NAME}.ini"

# Check if files already exist
HAS_ERRORS=0
if [ -f "$C_REF_FILE" ]; then
    echo -e "${RED}Error: Algorithm '$ALGO_NAME' already exists!${NC}"
    echo "File already exists: $C_REF_FILE"
    HAS_ERRORS=1
fi
if [ -f "$CL_FILE" ]; then
    [ $HAS_ERRORS -eq 0 ] && echo -e "${RED}Error: Algorithm '$ALGO_NAME' already exists!${NC}"
    echo "File already exists: $CL_FILE"
    HAS_ERRORS=1
fi
if [ -f "$CONFIG_FILE" ]; then
    [ $HAS_ERRORS -eq 0 ] && echo -e "${RED}Error: Algorithm '$ALGO_NAME' already exists!${NC}"
    echo "File already exists: $CONFIG_FILE"
    HAS_ERRORS=1
fi

if [ $HAS_ERRORS -eq 1 ]; then
    exit 1
fi

# Create directories
echo "Creating algorithm template for: $ALGO_NAME"
echo ""

mkdir -p "$C_REF_DIR"
echo -e "${GREEN}✓${NC} Created directory: $C_REF_DIR"

mkdir -p "$CL_DIR"
echo -e "${GREEN}✓${NC} Created directory: $CL_DIR"

mkdir -p "config"

################################################################################
# Generate C reference implementation
################################################################################
cat > "$C_REF_FILE" << EOF
#include "../../utils/safe_ops.h"
#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/verify.h"
#include <stddef.h>
#include <stdio.h>

/**
 * @brief ${ALGO_NAME_UPPER} reference implementation
 *
 * CPU implementation of ${ALGO_NAME} algorithm.
 * This serves as the ground truth for verifying GPU output.
 *
 * @param[in] params Operation parameters containing:
 *   - input: Input image buffer
 *   - output: Output image buffer
 *   - src_width, src_height: Source dimensions
 *   - dst_width, dst_height: Destination dimensions
 *   - custom_buffers: Optional custom buffers (NULL if none)
 */
void ${ALGO_NAME_C}_ref(const OpParams* params) {
    int y;
    int x;
    int width;
    int height;
    unsigned char* input;
    unsigned char* output;
    int total_pixels;

    if (params == NULL) {
        return;
    }

    /* Extract parameters */
    input = params->input;
    output = params->output;
    width = params->src_width;
    height = params->src_height;

    if ((input == NULL) || (output == NULL) || (width <= 0) || (height <= 0)) {
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!safe_mul_int(width, height, &total_pixels)) {
        return; /* Overflow detected */
    }

    /* TODO: Implement your algorithm here */
    /* Example: Simple copy operation */
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int index = y * width + x;
            if (index < total_pixels) {
                output[index] = input[index];
            }
        }
    }
}

/**
 * @brief Verify GPU result against reference
 *
 * Compares GPU output with reference implementation output.
 *
 * @param[in] params Operation parameters containing gpu_output and ref_output
 * @param[out] max_error Maximum absolute difference found
 * @return 1 if verification passed, 0 if failed
 */
int ${ALGO_NAME_C}_verify(const OpParams* params, float* max_error) {
    if (params == NULL) {
        return 0;
    }

    /* For exact match verification (e.g., for morphological operations) */
    /* Uncomment this if you need exact matching:
    int result = verify_exact_match(params->gpu_output, params->ref_output,
                                    params->dst_width, params->dst_height, 0);
    if (max_error != NULL) {
        *max_error = (result == 1) ? 0.0f : 1.0f;
    }
    return result;
    */

    /* For floating-point algorithms with tolerance */
    /* Allow small differences due to rounding (adjust tolerance as needed) */
    return verify_with_tolerance(params->gpu_output, params->ref_output,
                                params->dst_width, params->dst_height,
                                1.0f,      /* max_pixel_diff: 1 intensity level */
                                0.001f,    /* max_error_ratio: 0.1% of pixels can differ */
                                max_error);
}

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
int ${ALGO_NAME_C}_set_kernel_args(cl_kernel kernel,
                                cl_mem input_buf,
                                cl_mem output_buf,
                                const OpParams* params) {
    cl_uint arg_idx = 0U;

    if ((kernel == NULL) || (params == NULL)) {
        return -1;
    }

    /* Standard arguments: input, output, width, height */
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) != CL_SUCCESS) {
        return -1;
    }

    /* If your algorithm uses custom buffers, uncomment and modify this:
    if (params->custom_buffers == NULL) {
        (void)fprintf(stderr, "Error: ${ALGO_NAME_UPPER} requires custom buffers\\n");
        return -1;
    }
    CustomBuffers* custom = params->custom_buffers;
    if (custom->count < 1) {
        (void)fprintf(stderr, "Error: ${ALGO_NAME_UPPER} requires at least 1 custom buffer\\n");
        return -1;
    }

    // Set custom buffer arguments
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[0].buffer) != CL_SUCCESS) {
        return -1;
    }
    */

    return 0;
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * See src/utils/auto_registry.c for the registration code.
 */
EOF

echo -e "${GREEN}✓${NC} Created file: $C_REF_FILE"

################################################################################
# Generate OpenCL kernel
################################################################################
cat > "$CL_FILE" << EOF
/**
 * @file ${ALGO_NAME_C}0.cl
 * @brief ${ALGO_NAME_UPPER} OpenCL kernel implementation
 *
 * TODO: Add algorithm description here
 *
 * Kernel arguments:
 * @param input  Input image buffer
 * @param output Output image buffer
 * @param width  Image width in pixels
 * @param height Image height in pixels
 */

__kernel void ${ALGO_NAME_C}(__global const uchar* input,
                         __global uchar* output,
                         int width,
                         int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int index = y * width + x;

    /* TODO: Implement your algorithm here */
    /* Example: Simple copy operation */
    output[index] = input[index];
}
EOF

echo -e "${GREEN}✓${NC} Created file: $CL_FILE"

################################################################################
# Generate configuration file
################################################################################
cat > "$CONFIG_FILE" << EOF
# ${ALGO_NAME_UPPER} Algorithm Configuration
# TODO: Add algorithm description here
# Note: op_id is auto-derived from filename (${ALGO_NAME}.ini -> op_id = ${ALGO_NAME})

[image]
input = test_data/${ALGO_NAME}/input.bin
output = test_data/${ALGO_NAME}/output.bin
src_width = 1920
src_height = 1080
dst_width = 1920
dst_height = 1080

# Variant 0: Basic implementation using standard OpenCL API
[kernel.v0]
host_type = standard   # Options: "standard" (default) or "cl_extension"
kernel_file = src/${ALGO_NAME}/cl/${ALGO_NAME_C}0.cl
kernel_function = ${ALGO_NAME_C}
work_dim = 2
global_work_size = 1920,1088
local_work_size = 16,16

# Optional: Add custom buffers if needed
# Example: Custom buffer for algorithm-specific data
# [buffer.custom_data]
# type = READ_ONLY
# data_type = float
# num_elements = 100
# source_file = test_data/${ALGO_NAME}/custom_data.bin

# Optional: Add more kernel variants
# [kernel.v1]
# host_type = cl_extension
# kernel_file = src/${ALGO_NAME}/cl/${ALGO_NAME_C}1.cl
# kernel_function = ${ALGO_NAME_C}_optimized
# work_dim = 2
# global_work_size = 1920,1088
# local_work_size = 16,16
EOF

echo -e "${GREEN}✓${NC} Created file: $CONFIG_FILE"

################################################################################
# Print success message and next steps
################################################################################
echo ""
echo "======================================================================"
echo "Algorithm template created successfully!"
echo "======================================================================"
echo ""
echo "Next steps:"
echo "1. Implement the algorithm in: $C_REF_FILE"
echo "2. Implement the OpenCL kernel in: $CL_FILE"
echo "3. Configure parameters in: $CONFIG_FILE"
echo "4. Create test data directory: test_data/${ALGO_NAME}/"
echo "5. Generate test input: python3 scripts/generate_test_image.py"
echo "6. Rebuild the project: ./scripts/build.sh"
echo "7. Run your algorithm: ./build/opencl_host ${ALGO_NAME} 0"
echo ""
echo "See docs/ADD_NEW_ALGO.md for detailed implementation guide."
