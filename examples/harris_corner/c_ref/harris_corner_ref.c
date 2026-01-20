#include <math.h>
#include <stddef.h>

#include "op_interface.h"
#include "op_registry.h"
#include "utils/safe_ops.h"

/**
 * @file harris_corner_ref.c
 * @brief Harris Corner Detection Reference Implementation
 *
 * Based on OpenCV's cornerHarris implementation.
 * Implements the Harris corner detector algorithm:
 * 1. Compute gradients using Sobel operator
 * 2. Build structure tensor with Gaussian weighting
 * 3. Compute Harris response: R = det(M) - k * trace(M)^2
 *
 * OpenCV Reference:
 *   - C++ implementation: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/corner.cpp
 *     (see cornerHarris, cornerEigenValsVecs, calcHarris)
 *   - Sobel operator: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/deriv.cpp
 */

/**
 * @brief Compute Sobel gradients at a pixel
 *
 * Uses 3x3 Sobel operator as in OpenCV (aperture_size=3).
 * Sobel Gx: [-1 0 1; -2 0 2; -1 0 1]
 * Sobel Gy: [-1 -2 -1; 0 0 0; 1 2 1]
 *
 * @param input Input image
 * @param x,y   Pixel coordinates
 * @param width Image width
 * @param Ix    Output: horizontal gradient
 * @param Iy    Output: vertical gradient
 */
static void compute_sobel_gradients(const unsigned char* input,
                                    int x, int y, int width,
                                    float* Ix, float* Iy) {
    /* Sobel horizontal gradient (Gx) */
    *Ix = (float)input[(y-1) * width + (x+1)] - (float)input[(y-1) * width + (x-1)]
        + 2.0f * ((float)input[y * width + (x+1)] - (float)input[y * width + (x-1)])
        + (float)input[(y+1) * width + (x+1)] - (float)input[(y+1) * width + (x-1)];

    /* Sobel vertical gradient (Gy) */
    *Iy = (float)input[(y+1) * width + (x-1)] - (float)input[(y-1) * width + (x-1)]
        + 2.0f * ((float)input[(y+1) * width + x] - (float)input[(y-1) * width + x])
        + (float)input[(y+1) * width + (x+1)] - (float)input[(y-1) * width + (x+1)];
}

/**
 * @brief Harris Corner Detection reference implementation
 *
 * CPU implementation of Harris corner detector following OpenCV's approach.
 * Detects corners by analyzing local gradient structure.
 *
 * Algorithm:
 * 1. Compute image gradients Ix, Iy using Sobel operator
 * 2. Build structure tensor M = [Sxx Sxy; Sxy Syy] with Gaussian weighting
 * 3. Compute Harris response: R = det(M) - k * trace(M)^2
 *
 * Reference: OpenCV modules/imgproc/src/corner.cpp - cornerEigenValsVecs()
 *
 * @param[in] params Operation parameters containing:
 *   - input: Input grayscale image
 *   - output: Harris response map (float)
 *   - src_width, src_height: Image dimensions
 */
void HarrisCornerRef(const OpParams* params) {
    int y;
    int x;
    int wy;
    int wx;
    int width;
    int height;
    unsigned char* input;
    float* output;
    int total_pixels;
    float k;

    /* Gaussian weights for 5x5 window (sigma ~= 1.0)
     * OpenCV uses boxFilter by default, but Gaussian gives better results
     * These weights sum to 1.0 */
    static const float gauss[5][5] = {
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f}
    };

    if (params == NULL) {
        return;
    }

    /* Extract parameters */
    input = params->input;
    output = (float*)params->output;
    width = params->src_width;
    height = params->src_height;
    k = 0.04f; /* Harris parameter (OpenCV default) */

    if ((input == NULL) || (output == NULL) ||
        (width <= 0) || (height <= 0)) {
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!SafeMulInt(width, height, &total_pixels)) {
        return;
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int idx = y * width + x;
            float Sxx = 0.0f;
            float Syy = 0.0f;
            float Sxy = 0.0f;
            float det;
            float trace;
            float response;

            /* Skip border pixels (need 3 pixels for gradient + 2 for window) */
            if ((x < 3) || (x >= width - 3) ||
                (y < 3) || (y >= height - 3)) {
                output[idx] = 0.0f;
                continue;
            }

            /* Accumulate weighted gradient products (structure tensor)
             * M = [Sxx Sxy; Sxy Syy] */
            for (wy = -2; wy <= 2; wy++) {
                for (wx = -2; wx <= 2; wx++) {
                    int px = x + wx;
                    int py = y + wy;
                    float Ix;
                    float Iy;
                    float w;

                    /* Compute gradients using Sobel operator (OpenCV default) */
                    compute_sobel_gradients(input, px, py, width, &Ix, &Iy);

                    /* Gaussian weight */
                    w = gauss[wy + 2][wx + 2];

                    /* Accumulate structure tensor */
                    Sxx += w * Ix * Ix;
                    Syy += w * Iy * Iy;
                    Sxy += w * Ix * Iy;
                }
            }

            /* Compute Harris response (OpenCV formula)
             * R = det(M) - k * trace(M)^2
             * det(M) = Sxx * Syy - Sxy^2
             * trace(M) = Sxx + Syy */
            det = Sxx * Syy - Sxy * Sxy;
            trace = Sxx + Syy;
            response = det - k * trace * trace;

            output[idx] = response;
        }
    }
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * Auto-generated by scripts/generate_registry.sh which scans for *_ref.c files.
 */
