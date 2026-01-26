/**
 * @file svd_2.cl
 * @brief SVD patch kernel - Gaussian-weighted structure tensor over window
 *
 * Computes SVD of accumulated structure tensor over a 3x3 window with
 * Gaussian weighting, similar to OpenCV's cornerEigenValsVecs.
 *
 * OpenCV Reference:
 *   - Eigenvalue computation: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/corner.cpp
 *     (see calcMinEigenVal, calcEigenValsVecs)
 */

/**
 * @brief Compute SVD of a 2x2 symmetric matrix using closed-form solution
 *
 * For symmetric matrix M = [a b; b c], compute eigenvalues (which are
 * singular values for symmetric matrices) and eigenvector angle.
 *
 * @param a,b,c     Matrix elements [a b; b c] (symmetric)
 * @param s1        Output: larger singular value (sqrt of larger eigenvalue)
 * @param s2        Output: smaller singular value (sqrt of smaller eigenvalue)
 * @param theta     Output: rotation angle of principal eigenvector
 */
void svd_2x2_symmetric(float a, float b, float c,
                       float* s1, float* s2, float* theta) {
    float trace = a + c;
    float diff = a - c;
    float discriminant = sqrt(diff * diff * 0.25f + b * b);

    float lambda1 = trace * 0.5f + discriminant;
    float lambda2 = trace * 0.5f - discriminant;

    *s1 = sqrt(fmax(lambda1, 0.0f));
    *s2 = sqrt(fmax(lambda2, 0.0f));

    if (fabs(b) > 1e-10f) {
        *theta = 0.5f * atan2(2.0f * b, a - c);
    } else if (a >= c) {
        *theta = 0.0f;
    } else {
        *theta = M_PI_2_F;
    }
}

/**
 * @brief Sobel gradient computation (3x3 Scharr-like operator)
 *
 * Computes image gradients using Scharr operator for better accuracy.
 * Scharr kernel: [-3 0 3; -10 0 10; -3 0 3] / 32
 */
void compute_scharr_gradients(__global const uchar* input,
                              int x, int y, int width,
                              float* Ix, float* Iy) {
    const float k1 = 3.0f / 32.0f;
    const float k2 = 10.0f / 32.0f;

    *Ix = k1 * ((float)input[(y-1) * width + (x+1)] - (float)input[(y-1) * width + (x-1)])
        + k2 * ((float)input[y * width + (x+1)] - (float)input[y * width + (x-1)])
        + k1 * ((float)input[(y+1) * width + (x+1)] - (float)input[(y+1) * width + (x-1)]);

    *Iy = k1 * ((float)input[(y+1) * width + (x-1)] - (float)input[(y-1) * width + (x-1)])
        + k2 * ((float)input[(y+1) * width + x] - (float)input[(y-1) * width + x])
        + k1 * ((float)input[(y+1) * width + (x+1)] - (float)input[(y-1) * width + (x+1)]);
}

/**
 * @brief SVD-based image patch decomposition with Gaussian weighting
 *
 * Computes SVD of accumulated structure tensor over a 3x3 window.
 * Uses Gaussian weighting similar to OpenCV's cornerEigenValsVecs.
 *
 * @param[in]  input      Input grayscale image buffer (uchar, size: width * height)
 * @param[out] s1_out     First (larger) singular value output (float, size: width * height)
 * @param[out] s2_out     Second (smaller) singular value output (float, size: width * height)
 * @param[out] coherence  Anisotropy measure (s1-s2)/(s1+s2), range [0,1] (float, size: width * height)
 * @param[in]  width      Image width in pixels
 * @param[in]  height     Image height in pixels
 */
__kernel void svd_patch(__global const uchar* input,
                        __global float* s1_out,
                        __global float* s2_out,
                        __global float* coherence,
                        int width,
                        int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    /* Skip border pixels where we can't compute 3x3 patch with gradients */
    if (x < 2 || x >= width - 2 || y < 2 || y >= height - 2) {
        s1_out[idx] = 0.0f;
        s2_out[idx] = 0.0f;
        coherence[idx] = 0.0f;
        return;
    }

    /* Gaussian weights for 3x3 window (sigma ~= 0.85)
     * Similar to OpenCV's approach but with Gaussian instead of box filter */
    const float gauss[3][3] = {
        {0.0625f, 0.125f, 0.0625f},
        {0.125f,  0.25f,  0.125f},
        {0.0625f, 0.125f, 0.0625f}
    };

    /* Accumulate weighted structure tensor over 3x3 window */
    float Sxx = 0.0f;
    float Syy = 0.0f;
    float Sxy = 0.0f;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int px = x + dx;
            int py = y + dy;

            /* Compute Scharr gradients */
            float Ix, Iy;
            compute_scharr_gradients(input, px, py, width, &Ix, &Iy);

            /* Apply Gaussian weight */
            float w = gauss[dy + 1][dx + 1];

            Sxx += w * Ix * Ix;
            Syy += w * Iy * Iy;
            Sxy += w * Ix * Iy;
        }
    }

    /* SVD of symmetric 2x2 structure tensor */
    float s1, s2, theta;
    svd_2x2_symmetric(Sxx, Sxy, Syy, &s1, &s2, &theta);

    s1_out[idx] = s1;
    s2_out[idx] = s2;

    /* Coherence measures how anisotropic the local structure is
     * coherence = (s1 - s2) / (s1 + s2 + epsilon)
     * Range: [0, 1], where 1 = highly directional, 0 = isotropic */
    float epsilon = 1e-6f;
    coherence[idx] = (s1 - s2) / (s1 + s2 + epsilon);
}
