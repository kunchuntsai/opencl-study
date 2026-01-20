#include <math.h>
#include <stddef.h>

#include "op_interface.h"
#include "op_registry.h"
#include "utils/safe_ops.h"

/**
 * @file svd_ref.c
 * @brief SVD Reference Implementation
 *
 * Based on OpenCV's SVD approach.
 * Uses closed-form solution for 2x2 symmetric matrices (structure tensors).
 *
 * OpenCV Reference:
 *   - SVD implementation: https://github.com/opencv/opencv/blob/4.x/modules/core/src/lapack.cpp
 *     (see JacobiSVDImpl_, SVD::compute)
 *   - Scharr derivatives: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/deriv.cpp
 *     (see Scharr, getScharrKernels)
 */

/**
 * @brief Compute Scharr gradients at a pixel
 *
 * Uses Scharr operator for better rotational symmetry than Sobel.
 * Kernel: [-3 0 3; -10 0 10; -3 0 3] / 32
 * Reference: OpenCV imgproc/src/deriv.cpp
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
    /* Scharr kernel coefficients (normalized) */
    const float k1 = 3.0f / 32.0f;
    const float k2 = 10.0f / 32.0f;

    /* Horizontal gradient (Scharr) */
    *Ix = k1 * ((float)input[(y-1) * width + (x+1)] - (float)input[(y-1) * width + (x-1)])
        + k2 * ((float)input[y * width + (x+1)] - (float)input[y * width + (x-1)])
        + k1 * ((float)input[(y+1) * width + (x+1)] - (float)input[(y+1) * width + (x-1)]);

    /* Vertical gradient (Scharr) */
    *Iy = k1 * ((float)input[(y+1) * width + (x-1)] - (float)input[(y-1) * width + (x-1)])
        + k2 * ((float)input[(y+1) * width + x] - (float)input[(y-1) * width + x])
        + k1 * ((float)input[(y+1) * width + (x+1)] - (float)input[(y-1) * width + (x+1)]);
}

/**
 * @brief Compute SVD of a 2x2 symmetric matrix
 *
 * For symmetric matrix M = [a b; b c], compute singular values and rotation angle.
 * For symmetric matrices, SVD is equivalent to eigendecomposition:
 *   M = V * S * V^T
 *
 * Based on OpenCV's approach for eigenvalue computation.
 *
 * @param a,b,c  Matrix elements [a b; b c]
 * @param s1     Output: larger singular value
 * @param s2     Output: smaller singular value
 * @param theta  Output: rotation angle
 */
static void svd_2x2_symmetric(float a, float b, float c,
                              float* s1, float* s2, float* theta) {
    float trace;
    float diff;
    float discriminant;
    float lambda1;
    float lambda2;

    /* For symmetric 2x2 matrix [a b; b c], eigenvalues are:
       lambda = (a+c)/2 +/- sqrt(((a-c)/2)^2 + b^2) */
    trace = a + c;
    diff = a - c;
    discriminant = sqrtf(diff * diff * 0.25f + b * b);

    /* Eigenvalues (always non-negative for covariance matrices) */
    lambda1 = trace * 0.5f + discriminant;
    lambda2 = trace * 0.5f - discriminant;

    /* Singular values are sqrt of eigenvalues */
    *s1 = sqrtf(fmaxf(lambda1, 0.0f));
    *s2 = sqrtf(fmaxf(lambda2, 0.0f));

    /* Compute rotation angle from eigenvector
     * Using atan2 for numerical stability (OpenCV style) */
    if (fabsf(b) > 1e-10f) {
        *theta = 0.5f * atan2f(2.0f * b, a - c);
    } else if (a >= c) {
        *theta = 0.0f;
    } else {
        *theta = (float)M_PI / 2.0f;
    }
}

/**
 * @brief SVD reference implementation
 *
 * CPU implementation of SVD for 2x2 structure tensors.
 * Computes singular values and orientation for each pixel.
 * Based on OpenCV's cornerEigenValsVecs approach.
 *
 * @param[in] params Operation parameters containing:
 *   - input: Input grayscale image
 *   - output: Singular values and angles (stored in custom buffers)
 *   - src_width, src_height: Image dimensions
 */
void SvdRef(const OpParams* params) {
    int y;
    int x;
    int width;
    int height;
    unsigned char* input;
    float* sigma1;
    float* sigma2;
    float* angle;
    int total_pixels;

    if (params == NULL) {
        return;
    }

    /* Extract parameters */
    input = params->input;
    width = params->src_width;
    height = params->src_height;

    if ((input == NULL) || (width <= 0) || (height <= 0)) {
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!SafeMulInt(width, height, &total_pixels)) {
        return;
    }

    /* Get output buffers from custom buffers */
    if ((params->custom_buffers == NULL) || (params->custom_buffers->count < 3)) {
        return;
    }

    sigma1 = (float*)params->custom_buffers->buffers[0].host_data;
    sigma2 = (float*)params->custom_buffers->buffers[1].host_data;
    angle = (float*)params->custom_buffers->buffers[2].host_data;

    if ((sigma1 == NULL) || (sigma2 == NULL) || (angle == NULL)) {
        return;
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int idx = y * width + x;
            float Ix;
            float Iy;
            float Sxx;
            float Sxy;
            float Syy;
            float s1;
            float s2;
            float theta;

            /* Skip border pixels (need 1 pixel for Scharr gradients) */
            if ((x < 1) || (x >= width - 1) ||
                (y < 1) || (y >= height - 1)) {
                sigma1[idx] = 0.0f;
                sigma2[idx] = 0.0f;
                angle[idx] = 0.0f;
                continue;
            }

            /* Compute gradients using Scharr operator (OpenCV style) */
            compute_scharr_gradients(input, x, y, width, &Ix, &Iy);

            /* Build structure tensor M = [Sxx Sxy; Sxy Syy] */
            Sxx = Ix * Ix;
            Sxy = Ix * Iy;
            Syy = Iy * Iy;

            /* Compute SVD of symmetric structure tensor */
            svd_2x2_symmetric(Sxx, Sxy, Syy, &s1, &s2, &theta);

            sigma1[idx] = s1;
            sigma2[idx] = s2;
            angle[idx] = theta;
        }
    }
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * Auto-generated by scripts/generate_registry.sh which scans for *_ref.c files.
 */
