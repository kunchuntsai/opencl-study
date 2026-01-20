#include <math.h>
#include <stddef.h>

#include "op_interface.h"
#include "op_registry.h"
#include "utils/safe_ops.h"

/**
 * @file lucas_kanade_ref.c
 * @brief Lucas-Kanade Optical Flow Reference Implementation
 *
 * Based on OpenCV's Lucas-Kanade implementation.
 * Computes dense optical flow between two consecutive frames.
 *
 * Algorithm:
 * 1. Compute spatial gradients Ix, Iy using Scharr operator
 * 2. Compute temporal gradient It = I2 - I1
 * 3. Build structure tensor A and vector b over a window
 * 4. Solve 2x2 linear system: A * v = b
 *
 * OpenCV Reference:
 *   - C++ implementation: https://github.com/opencv/opencv/blob/4.x/modules/video/src/lkpyramid.cpp
 *     (see calcOpticalFlowPyrLK, LKTrackerInvoker, calcScharrDeriv)
 *   - Scharr derivatives: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/deriv.cpp
 */

/* Constants for tracking quality */
#define MIN_EIGENVAL_THRESHOLD 1e-4f

/**
 * @brief Compute Scharr gradients at a pixel
 *
 * Uses 3x3 Scharr operator for better accuracy than central differences.
 * Scharr Gx: [-3 0 3; -10 0 10; -3 0 3]
 * Scharr Gy: [-3 -10 -3; 0 0 0; 3 10 3]
 *
 * Reference: OpenCV video/src/lkpyramid.cpp - calcScharrDeriv()
 *
 * @param input Input image
 * @param x,y   Pixel coordinates
 * @param width Image width
 * @param Ix    Output: horizontal gradient
 * @param Iy    Output: vertical gradient
 */
static void compute_scharr_gradients(const unsigned char* input,
                                     int x, int y, int width,
                                     float* Ix, float* Iy) {
    /* Scharr horizontal gradient (Gx) */
    *Ix = 3.0f * ((float)input[(y-1) * width + (x+1)] - (float)input[(y-1) * width + (x-1)])
        + 10.0f * ((float)input[y * width + (x+1)] - (float)input[y * width + (x-1)])
        + 3.0f * ((float)input[(y+1) * width + (x+1)] - (float)input[(y+1) * width + (x-1)]);

    /* Scharr vertical gradient (Gy) */
    *Iy = 3.0f * ((float)input[(y+1) * width + (x-1)] - (float)input[(y-1) * width + (x-1)])
        + 10.0f * ((float)input[(y+1) * width + x] - (float)input[(y-1) * width + x])
        + 3.0f * ((float)input[(y+1) * width + (x+1)] - (float)input[(y-1) * width + (x+1)]);
}

/**
 * @brief Lucas-Kanade Optical Flow reference implementation
 *
 * CPU implementation of Lucas-Kanade optical flow algorithm following
 * OpenCV's approach. Computes motion vectors between two consecutive frames.
 *
 * Reference: OpenCV modules/video/src/lkpyramid.cpp
 *
 * @param[in] params Operation parameters containing:
 *   - input: Previous frame (grayscale)
 *   - output: Current frame (grayscale) - NOTE: uses output as second input
 *   - src_width, src_height: Frame dimensions
 */
void LucasKanadeRef(const OpParams* params) {
    int y;
    int x;
    int wy;
    int wx;
    int width;
    int height;
    unsigned char* prev_frame;
    unsigned char* curr_frame;
    float* flow_x;
    float* flow_y;
    int total_pixels;
    int window_size;
    int half_win;

    if (params == NULL) {
        return;
    }

    /* Extract parameters */
    prev_frame = params->input;
    curr_frame = params->output; /* Using output as second input for now */
    width = params->src_width;
    height = params->src_height;
    window_size = 5; /* Default window size (OpenCV typically uses 21) */
    half_win = window_size / 2;

    if ((prev_frame == NULL) || (curr_frame == NULL) ||
        (width <= 0) || (height <= 0)) {
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!SafeMulInt(width, height, &total_pixels)) {
        return;
    }

    /* Get output buffers from custom buffers */
    if ((params->custom_buffers == NULL) || (params->custom_buffers->count < 2)) {
        return;
    }

    flow_x = (float*)params->custom_buffers->buffers[0].host_data;
    flow_y = (float*)params->custom_buffers->buffers[1].host_data;

    if ((flow_x == NULL) || (flow_y == NULL)) {
        return;
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int idx = y * width + x;
            float A11 = 0.0f;  /* sum_Ix2 */
            float A22 = 0.0f;  /* sum_Iy2 */
            float A12 = 0.0f;  /* sum_IxIy */
            float b1 = 0.0f;   /* -sum_IxIt */
            float b2 = 0.0f;   /* -sum_IyIt */
            float det;
            float min_eigenval;
            float inv_det;
            float vx;
            float vy;

            /* Skip border pixels */
            if ((x < half_win + 1) || (x >= width - half_win - 1) ||
                (y < half_win + 1) || (y >= height - half_win - 1)) {
                flow_x[idx] = 0.0f;
                flow_y[idx] = 0.0f;
                continue;
            }

            /* Accumulate structure tensor over window
             * A = [A11 A12]    b = [b1]
             *     [A12 A22]        [b2] */
            for (wy = -half_win; wy <= half_win; wy++) {
                for (wx = -half_win; wx <= half_win; wx++) {
                    int px = x + wx;
                    int py = y + wy;
                    float Ix;
                    float Iy;
                    float It;

                    /* Spatial gradients using Scharr operator (OpenCV style) */
                    compute_scharr_gradients(prev_frame, px, py, width, &Ix, &Iy);

                    /* Temporal gradient */
                    It = (float)curr_frame[py * width + px] -
                         (float)prev_frame[py * width + px];

                    /* Accumulate */
                    A11 += Ix * Ix;
                    A22 += Iy * Iy;
                    A12 += Ix * Iy;
                    b1 -= Ix * It;
                    b2 -= Iy * It;
                }
            }

            /* Solve 2x2 system using Cramer's rule
             * det(A) = A11*A22 - A12*A12
             * Check for trackability using minimum eigenvalue (OpenCV approach) */
            det = A11 * A22 - A12 * A12;
            min_eigenval = (A11 + A22 - sqrtf((A11 - A22) * (A11 - A22) + 4.0f * A12 * A12)) * 0.5f;

            if ((fabsf(det) < MIN_EIGENVAL_THRESHOLD) ||
                (min_eigenval < MIN_EIGENVAL_THRESHOLD)) {
                /* Flat region or aperture problem */
                flow_x[idx] = 0.0f;
                flow_y[idx] = 0.0f;
            } else {
                /* Compute flow using Cramer's rule */
                inv_det = 1.0f / det;
                vx = (A22 * b1 - A12 * b2) * inv_det;
                vy = (A11 * b2 - A12 * b1) * inv_det;
                flow_x[idx] = vx;
                flow_y[idx] = vy;
            }
        }
    }
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * Auto-generated by scripts/generate_registry.sh which scans for *_ref.c files.
 */
