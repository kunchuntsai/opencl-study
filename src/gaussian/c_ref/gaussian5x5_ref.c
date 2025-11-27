#include "../../utils/safe_ops.h"
#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/verify.h"
#include <stddef.h>
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

/* Algorithm-specific buffers structure */
typedef struct {
    cl_mem weights_buf;  /* Buffer holding the Gaussian kernel weights */
} Gaussian5x5Buffers;

/* Create algorithm-specific buffers - demonstrates CreateBuffersFunc usage */
void* gaussian5x5_create_buffers(cl_context context,
                                 const OpParams* params,
                                 const unsigned char* input,
                                 size_t img_size) {
    Gaussian5x5Buffers* buffers;
    cl_int err;

    /* Gaussian 5x5 kernel weights (same as in the C reference) */
    static const float kernel_weights[25] = {
        1.0f, 4.0f, 6.0f, 4.0f, 1.0f,
        4.0f, 16.0f, 24.0f, 16.0f, 4.0f,
        6.0f, 24.0f, 36.0f, 24.0f, 6.0f,
        4.0f, 16.0f, 24.0f, 16.0f, 4.0f,
        1.0f, 4.0f, 6.0f, 4.0f, 1.0f
    };

    /* Unused parameters - kept for interface compliance */
    (void)params;
    (void)input;
    (void)img_size;

    if (context == NULL) {
        return NULL;
    }

    /* Allocate buffers structure */
    buffers = (Gaussian5x5Buffers*)malloc(sizeof(Gaussian5x5Buffers));
    if (buffers == NULL) {
        return NULL;
    }

    /* Create read-only buffer for kernel weights */
    buffers->weights_buf = clCreateBuffer(context,
                                         CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         sizeof(kernel_weights),
                                         (void*)kernel_weights,
                                         &err);
    if (err != CL_SUCCESS || buffers->weights_buf == NULL) {
        free(buffers);
        return NULL;
    }

    return buffers;
}

/* Destroy algorithm-specific buffers - demonstrates DestroyBuffersFunc usage */
void gaussian5x5_destroy_buffers(void* algo_buffers) {
    Gaussian5x5Buffers* buffers;

    if (algo_buffers == NULL) {
        return;
    }

    buffers = (Gaussian5x5Buffers*)algo_buffers;

    /* Release OpenCL buffer */
    if (buffers->weights_buf != NULL) {
        (void)clReleaseMemObject(buffers->weights_buf);
    }

    /* Free structure */
    free(buffers);
}

/* Kernel argument setter - sets 5 arguments including weights buffer */
int gaussian5x5_set_kernel_args(cl_kernel kernel,
                                       cl_mem input_buf,
                                       cl_mem output_buf,
                                       const OpParams* params,
                                       void* algo_buffers) {
    cl_uint arg_idx = 0U;
    Gaussian5x5Buffers* buffers;

    if ((kernel == NULL) || (params == NULL) || (algo_buffers == NULL)) {
        return -1;
    }

    buffers = (Gaussian5x5Buffers*)algo_buffers;

    /* Set all 5 kernel arguments:
     * 0: input buffer
     * 1: output buffer
     * 2: weights buffer (algorithm-specific)
     * 3: width
     * 4: height
     */
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &buffers->weights_buf) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) != CL_SUCCESS) {
        return -1;
    }
    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) != CL_SUCCESS) {
        return -1;
    }

    return 0;
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * The auto-generation script has been configured to use the custom buffer functions
 * (gaussian5x5_create_buffers and gaussian5x5_destroy_buffers) instead of NULL.
 *
 * See src/utils/auto_registry.c for the registration code.
 */
