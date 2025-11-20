#include "dilate3x3_ref.h"
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
static unsigned char get_pixel_safe(const unsigned char* input, int x, int y,
                                   int width, int height) {
    int clamped_x;
    int clamped_y;
    int index;

    if ((input == NULL) || (width <= 0) || (height <= 0)) {
        return 0U;
    }

    clamped_x = clamp_coord(x, width);
    clamped_y = clamp_coord(y, height);
    index = clamped_y * width + clamped_x;

    return input[index];
}

void dilate3x3_ref(unsigned char* input, unsigned char* output,
                   int width, int height) {
    int y;
    int x;
    int dy;
    int dx;
    int ny;
    int nx;
    unsigned char max_val;
    unsigned char val;
    int output_index;

    if ((input == NULL) || (output == NULL) || (width <= 0) || (height <= 0)) {
        return;
    }

    /* Handle borders by replication */
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            max_val = 0U;

            /* 3x3 window */
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    ny = y + dy;
                    nx = x + dx;

                    /* Get pixel value with bounds checking */
                    val = get_pixel_safe(input, nx, ny, width, height);
                    if (val > max_val) {
                        max_val = val;
                    }
                }
            }

            output_index = y * width + x;

            /* MISRA-C:2023 Rule 18.1: Bounds check before write */
            if (output_index < (width * height)) {
                output[output_index] = max_val;
            }
        }
    }
}
