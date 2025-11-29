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
    /* Gaussian 5x5 kernel (normalized) */
    /* MISRA-C:2023 Rule 9.5: Explicit array initialization */
    static const float kernel[5][5] = {
        {1.0f, 4.0f, 6.0f, 4.0f, 1.0f},
        {4.0f, 16.0f, 24.0f, 16.0f, 4.0f},
        {6.0f, 24.0f, 36.0f, 24.0f, 6.0f},
        {4.0f, 16.0f, 24.0f, 16.0f, 4.0f},
        {1.0f, 4.0f, 6.0f, 4.0f, 1.0f}
    };
    static const float kernel_sum = 256.0f;
    int y;
    int x;
    int dy;
    int dx;
    int ny;
    int nx;
    float sum;
    float weight;
    int pixel_val;
    int output_index;
    int total_pixels;
    unsigned char result;
    int width;
    int height;
    unsigned char* input;
    unsigned char* output;

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

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sum = 0.0f;

            /* 5x5 window */
            for (dy = -2; dy <= 2; dy++) {
                for (dx = -2; dx <= 2; dx++) {
                    ny = y + dy;
                    nx = x + dx;

                    /* Get pixel value with bounds checking */
                    pixel_val = get_pixel_safe(input, nx, ny, width, height);
                    weight = kernel[dy + 2][dx + 2];
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


/* Runtime structure to hold created custom buffers (must match main.c) */
typedef struct {
    cl_mem buffer;
    unsigned char* host_data;
} RuntimeBuffer;

typedef struct {
    RuntimeBuffer buffers[8];  /* MAX_CUSTOM_BUFFERS */
    int count;
} CustomBuffers;

/* Kernel argument setter - sets kernel arguments to match kernel signature
 *
 * Kernel signature: gaussian5x5(input, output, tmp_buffer, width, height, kernel_x, kernel_y)
 *
 * Arguments mapping:
 *   arg 0: input
 *   arg 1: output
 *   arg 2: tmp_buffer (custom_buffers[0])
 *   arg 3: width (scalar)
 *   arg 4: height (scalar)
 *   arg 5: kernel_x (custom_buffers[1])
 *   arg 6: kernel_y (custom_buffers[2])
 */
int gaussian5x5_set_kernel_args(cl_kernel kernel,
                                       cl_mem input_buf,
                                       cl_mem output_buf,
                                       const OpParams* params) {
    CustomBuffers* custom_buffers;

    if ((kernel == NULL) || (params == NULL)) {
        return -1;
    }

    /* Verify we have the expected number of custom buffers */
    if (params->algo_params == NULL) {
        (void)fprintf(stderr, "Error: Gaussian kernel requires custom buffers\n");
        return -1;
    }
    custom_buffers = (CustomBuffers*)params->algo_params;
    if (custom_buffers->count != 3) {
        (void)fprintf(stderr, "Error: Gaussian kernel requires exactly 3 custom buffers (got %d)\n",
                     custom_buffers->count);
        return -1;
    }

    /* Set arguments in exact order matching kernel signature */

    /* arg 0: input */
    if (clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {
        return -1;
    }

    /* arg 1: output */
    if (clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {
        return -1;
    }

    /* arg 2: tmp_buffer (first custom buffer from config) */
    if (clSetKernelArg(kernel, 2, sizeof(cl_mem), &custom_buffers->buffers[0].buffer) != CL_SUCCESS) {
        return -1;
    }

    /* arg 3: width */
    if (clSetKernelArg(kernel, 3, sizeof(int), &params->src_width) != CL_SUCCESS) {
        return -1;
    }

    /* arg 4: height */
    if (clSetKernelArg(kernel, 4, sizeof(int), &params->src_height) != CL_SUCCESS) {
        return -1;
    }

    /* arg 5: kernel_x (second custom buffer from config) */
    if (clSetKernelArg(kernel, 5, sizeof(cl_mem), &custom_buffers->buffers[1].buffer) != CL_SUCCESS) {
        return -1;
    }

    /* arg 6: kernel_y (third custom buffer from config) */
    if (clSetKernelArg(kernel, 6, sizeof(cl_mem), &custom_buffers->buffers[2].buffer) != CL_SUCCESS) {
        return -1;
    }

    return 0;
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * See src/utils/auto_registry.c for the registration code.
 */
