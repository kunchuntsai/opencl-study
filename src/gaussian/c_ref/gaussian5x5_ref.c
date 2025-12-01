#include "../../utils/safe_ops.h"
#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/verify.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* Helper function to clamp coordinates to image bounds */
static int clamp_coord(int coord, int max_coord) {
    int result;

    if (coord < 0) {
        result = 0;
    } else if (coord >= max_coord) {
        result = max_coord - 1;
    } else {
        result = coord;
    }

    return result;
}

/* MISRA-C:2023 Rule 18.1: Add bounds checking for array access */
static int get_pixel_safe(const unsigned char* input, int x, int y,
                         int width, int height) {
    int clamped_x;
    int clamped_y;
    int index;

    if ((input == NULL) || (width <= 0) || (height <= 0)) {
        return 0;
    }

    clamped_x = clamp_coord(x, width);
    clamped_y = clamp_coord(y, height);
    index = clamped_y * width + clamped_x;

    return (int)input[index];
}

void gaussian5x5_ref(const OpParams* params) {
    /* Separable Gaussian 5x5 using 1D kernels from custom buffers */
    /* This matches the OpenCL implementation which uses kernel_x and kernel_y */
    int y;
    int x;
    int dy;
    int dx;
    int ny;
    int nx;
    float sum;
    float kernel_sum;
    float weight;
    int pixel_val;
    int output_index;
    int total_pixels;
    unsigned char result;
    int width;
    int height;
    unsigned char* input;
    unsigned char* output;
    CustomBuffers* custom_buffers;
    const float* kernel_x;
    const float* kernel_y;

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

    /* Get 1D kernels from custom buffers */
    if (params->custom_buffers == NULL) {
        (void)fprintf(stderr, "Error: Gaussian reference requires custom buffers for kernel data\n");
        return;
    }
    custom_buffers = params->custom_buffers;

    /* Determine kernel buffer indices based on kernel_variant */
    /* Variant 0: tmp_buffer(0), kernel_x(1), kernel_y(2) */
    /* Variant 1: tmp_buffer(0), kernel_x(1), kernel_y(2) */
    /* Variant 2: tmp_buffer(0), tmp_buffer2(1), kernel_x(2), kernel_y(3) */
    int kernel_x_idx = (params->kernel_variant == 2) ? 2 : 1;
    int kernel_y_idx = (params->kernel_variant == 2) ? 3 : 2;
    int min_buffers = kernel_y_idx + 1;  /* Need at least up to kernel_y index */

    if (custom_buffers->count < min_buffers) {
        (void)fprintf(stderr, "Error: Gaussian reference requires at least %d custom buffers (got %d)\n",
                     min_buffers, custom_buffers->count);
        return;
    }

    /* Get kernel_x and kernel_y from appropriate buffer indices */
    /* (tmp_buffer(s) are only used by GPU, not by reference implementation) */
    kernel_x = (const float*)custom_buffers->buffers[kernel_x_idx].host_data;
    kernel_y = (const float*)custom_buffers->buffers[kernel_y_idx].host_data;

    if ((kernel_x == NULL) || (kernel_y == NULL)) {
        (void)fprintf(stderr, "Error: Gaussian kernel data not loaded\n");
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!safe_mul_int(width, height, &total_pixels)) {
        return; /* Overflow detected */
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sum = 0.0f;
            kernel_sum = 0.0f;

            /* 5x5 window with separable convolution */
            /* 2D weight = kernel_y[i] * kernel_x[j] */
            for (dy = -2; dy <= 2; dy++) {
                for (dx = -2; dx <= 2; dx++) {
                    ny = y + dy;
                    nx = x + dx;

                    /* Get pixel value with bounds checking */
                    pixel_val = get_pixel_safe(input, nx, ny, width, height);

                    /* Compute 2D weight as outer product of 1D kernels */
                    weight = kernel_y[dy + 2] * kernel_x[dx + 2];
                    kernel_sum += weight;
                    sum += (float)pixel_val * weight;
                }
            }

            /* Convert to unsigned char with rounding */
            result = (unsigned char)((sum / kernel_sum) + 0.5f);
            output_index = y * width + x;

            /* MISRA-C:2023 Rule 18.1: Bounds check before write */
            if (output_index < total_pixels) {
                output[output_index] = result;
            }
        }
    }
}

/* Verification with tolerance for floating-point differences */
int gaussian5x5_verify(const OpParams* params, float* max_error) {
    if (params == NULL) {
        return 0;
    }
    /* Allow 1 intensity level difference due to rounding, pass if < 0.1% pixels differ */
    return verify_with_tolerance(params->gpu_output, params->ref_output,
                                params->dst_width, params->dst_height,
                                1.0f, 0.001f, max_error);
}


/* NOTE: RuntimeBuffer and CustomBuffers are now defined in utils/op_interface.h */

/* Kernel argument setter - sets kernel arguments based on kernel_variant
 *
 * Three kernel variants with different signatures:
 *
 * Variant 0 (gaussian5x5):
 *   arg 0: input
 *   arg 1: output
 *   arg 2: width
 *   arg 3: height
 *   arg 4: kernel_x (custom_buffers[1])
 *   arg 5: kernel_y (custom_buffers[2])
 *
 * Variant 1 (gaussian5x5_optimized):
 *   arg 0: input
 *   arg 1: output
 *   arg 2: tmp_buffer (custom_buffers[0])
 *   arg 3: tmp_buffer_size
 *   arg 4: width
 *   arg 5: height
 *   arg 6: kernel_x (custom_buffers[1])
 *   arg 7: kernel_y (custom_buffers[2])
 *
 * Variant 2 (gaussian5x5_horizontal):
 *   arg 0: input
 *   arg 1: output
 *   arg 2: tmp_buffer (custom_buffers[0])
 *   arg 3: tmp_buffer_size
 *   arg 4: tmp_buffer2 (custom_buffers[1])
 *   arg 5: tmp_buffer_size2
 *   arg 6: width
 *   arg 7: height
 *   arg 8: kernel_x (custom_buffers[2])
 *   arg 9: kernel_y (custom_buffers[3])
 */
int gaussian5x5_set_kernel_args(cl_kernel kernel,
                                       cl_mem input_buf,
                                       cl_mem output_buf,
                                       const OpParams* params) {
    CustomBuffers* custom_buffers;
    int arg_idx = 0;
    size_t tmp_buffer_size, tmp_buffer_size2;
    int kernel_x_idx, kernel_y_idx;

    if ((kernel == NULL) || (params == NULL)) {
        return -1;
    }

    /* Verify we have custom buffers */
    if (params->custom_buffers == NULL) {
        (void)fprintf(stderr, "Error: Gaussian kernel requires custom buffers\n");
        return -1;
    }
    custom_buffers = params->custom_buffers;

    /* Set common arguments (input and output) */
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {
        return -1;
    }

    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {
        return -1;
    }

    /* Set variant-specific arguments based on kernel_variant */
    switch (params->kernel_variant) {
        case 0:
            /* Variant 0: gaussian5x5(input, output, width, height, kernel_x, kernel_y) */
            /* No tmp buffers needed */
            if (custom_buffers->count < 3) {
                (void)fprintf(stderr, "Error: Variant 0 requires at least 3 buffers (got %d)\n",
                             custom_buffers->count);
                return -1;
            }
            kernel_x_idx = 1;  /* Skip tmp_buffer at index 0 */
            kernel_y_idx = 2;
            break;

        case 1:
            /* Variant 1: gaussian5x5_optimized(input, output, tmp_buffer, tmp_buffer_size, width, height, kernel_x, kernel_y) */
            if (custom_buffers->count < 3) {
                (void)fprintf(stderr, "Error: Variant 1 requires at least 3 buffers (got %d)\n",
                             custom_buffers->count);
                return -1;
            }

            tmp_buffer_size = custom_buffers->buffers[0].size_bytes;
            if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom_buffers->buffers[0].buffer) != CL_SUCCESS) {
                return -1;
            }
            if (clSetKernelArg(kernel, arg_idx++, sizeof(unsigned long), &tmp_buffer_size) != CL_SUCCESS) {
                return -1;
            }

            kernel_x_idx = 1;
            kernel_y_idx = 2;
            break;

        case 2:
            /* Variant 2: gaussian5x5_horizontal(input, output, tmp_buffer, tmp_buffer_size, tmp_buffer2, tmp_buffer_size2, width, height, kernel_x, kernel_y) */
            if (custom_buffers->count < 4) {
                (void)fprintf(stderr, "Error: Variant 2 requires at least 4 buffers (got %d)\n",
                             custom_buffers->count);
                return -1;
            }

            tmp_buffer_size = custom_buffers->buffers[0].size_bytes;
            tmp_buffer_size2 = custom_buffers->buffers[1].size_bytes;

            if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom_buffers->buffers[0].buffer) != CL_SUCCESS) {
                return -1;
            }
            if (clSetKernelArg(kernel, arg_idx++, sizeof(unsigned long), &tmp_buffer_size) != CL_SUCCESS) {
                return -1;
            }
            if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom_buffers->buffers[1].buffer) != CL_SUCCESS) {
                return -1;
            }
            if (clSetKernelArg(kernel, arg_idx++, sizeof(unsigned long), &tmp_buffer_size2) != CL_SUCCESS) {
                return -1;
            }

            kernel_x_idx = 2;
            kernel_y_idx = 3;
            break;

        default:
            (void)fprintf(stderr, "Error: Unknown kernel_variant %d\n", params->kernel_variant);
            return -1;
    }

    /* Set common arguments (width, height, kernel_x, kernel_y) */
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) != CL_SUCCESS) {
        return -1;
    }

    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) != CL_SUCCESS) {
        return -1;
    }

    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom_buffers->buffers[kernel_x_idx].buffer) != CL_SUCCESS) {
        return -1;
    }

    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom_buffers->buffers[kernel_y_idx].buffer) != CL_SUCCESS) {
        return -1;
    }

    return 0;
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * See src/utils/auto_registry.c for the registration code.
 */
