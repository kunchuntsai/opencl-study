#include "../../utils/safe_ops.h"
#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/verify.h"
#include <stddef.h>

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
static int gaussian5x5_verify(const OpParams* params, float* max_error) {
    if (params == NULL) {
        return 0;
    }
    /* Allow 1 intensity level difference due to rounding, pass if < 0.1% pixels differ */
    return verify_with_tolerance(params->gpu_output, params->ref_output,
                                params->dst_width, params->dst_height,
                                1.0f, 0.001f, max_error);
}

/* Auto-register algorithm using macro - eliminates boilerplate */
REGISTER_ALGORITHM(gaussian5x5, "Gaussian 5x5", gaussian5x5_ref, gaussian5x5_verify)
