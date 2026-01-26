/**
 * @file svd0.cl
 * @brief Singular Value Decomposition (SVD) OpenCL kernel implementation
 *
 * Computes SVD for 2x2 matrices extracted from image patches.
 * Based on OpenCV's SVD implementation approach with Jacobi rotation.
 * A = U * S * V^T where:
 *   - U: Left singular vectors (2x2 orthogonal matrix)
 *   - S: Singular values (2x1 diagonal, stored as 2 values)
 *   - V: Right singular vectors (2x2 orthogonal matrix)
 *
 * This is useful for:
 *   - Computing structure tensors in feature detection
 *   - Image compression on patches
 *   - Noise reduction
 *   - Computing eigenvalues for corner detection
 *
 * OpenCV Reference:
 *   - SVD implementation: https://github.com/opencv/opencv/blob/4.x/modules/core/src/lapack.cpp
 *     (see JacobiSVDImpl_, SVD::compute)
 *   - Eigenvalue computation: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/corner.cpp
 *     (see calcMinEigenVal, calcEigenValsVecs)
 */

/**
 * @brief Compute SVD of a 2x2 symmetric matrix using closed-form solution
 *
 * For symmetric matrix M = [a b; b c], compute eigenvalues (which are
 * singular values for symmetric matrices) and eigenvector angle.
 * This is equivalent to OpenCV's approach for symmetric matrices.
 *
 * Eigenvalues of [a b; b c]:
 *   lambda = (a+c)/2 +/- sqrt(((a-c)/2)^2 + b^2)
 *
 * @param a,b,c     Matrix elements [a b; b c] (symmetric)
 * @param s1        Output: larger singular value (sqrt of larger eigenvalue)
 * @param s2        Output: smaller singular value (sqrt of smaller eigenvalue)
 * @param theta     Output: rotation angle of principal eigenvector
 */
void svd_2x2_symmetric(float a, float b, float c,
                       float* s1, float* s2, float* theta) {
    /* For symmetric 2x2 matrix [a b; b c], eigenvalues are:
       lambda = (a+c)/2 +/- sqrt(((a-c)/2)^2 + b^2) */
    float trace = a + c;
    float diff = a - c;
    float discriminant = sqrt(diff * diff * 0.25f + b * b);

    /* Eigenvalues (always non-negative for covariance matrices) */
    float lambda1 = trace * 0.5f + discriminant;
    float lambda2 = trace * 0.5f - discriminant;

    /* Singular values are sqrt of eigenvalues */
    *s1 = sqrt(fmax(lambda1, 0.0f));
    *s2 = sqrt(fmax(lambda2, 0.0f));

    /* Compute rotation angle from eigenvector
     * For eigenvector of [a b; b c] with eigenvalue lambda1:
     * If b != 0: v = [b, lambda1 - a] or [lambda1 - c, b]
     * Using atan2 for numerical stability (OpenCV style) */
    if (fabs(b) > 1e-10f) {
        *theta = 0.5f * atan2(2.0f * b, a - c);
    } else if (a >= c) {
        *theta = 0.0f;
    } else {
        *theta = M_PI_2_F;
    }
}

/**
 * @brief Compute SVD of a general 2x2 matrix using Jacobi-like approach
 *
 * For matrix A = [a b; c d], compute SVD via A^T*A eigendecomposition.
 * Based on OpenCV's JacobiSVDImpl for small matrices.
 *
 * @param a,b,c,d   Matrix elements [a b; c d]
 * @param s1        Output: larger singular value
 * @param s2        Output: smaller singular value
 * @param theta     Output: rotation angle of V matrix
 */
void svd_2x2_general(float a, float b, float c, float d,
                     float* s1, float* s2, float* theta) {
    /* Compute A^T * A = [a c; b d] * [a b; c d] = [e f; f g]
     * This gives us a symmetric matrix whose eigenvalues are
     * the squared singular values of A */
    float e = a * a + c * c;  /* (A^T*A)[0,0] */
    float f = a * b + c * d;  /* (A^T*A)[0,1] = (A^T*A)[1,0] */
    float g = b * b + d * d;  /* (A^T*A)[1,1] */

    /* Use symmetric SVD on A^T*A */
    float trace = e + g;
    float diff = e - g;
    float discriminant = sqrt(diff * diff * 0.25f + f * f);

    /* Eigenvalues of A^T*A (squared singular values) */
    float lambda1 = trace * 0.5f + discriminant;
    float lambda2 = trace * 0.5f - discriminant;

    /* Singular values */
    *s1 = sqrt(fmax(lambda1, 0.0f));
    *s2 = sqrt(fmax(lambda2, 0.0f));

    /* Compute V rotation angle (right singular vectors)
     * OpenCV computes this from the eigenvectors of A^T*A */
    if (fabs(f) > 1e-10f) {
        *theta = 0.5f * atan2(2.0f * f, e - g);
    } else if (e >= g) {
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
 *
 * OpenCV Reference:
 *   - https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/deriv.cpp
 *     (see Scharr, getScharrKernels)
 */
void compute_scharr_gradients(__global const uchar* input,
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
 * @brief Compute per-pixel SVD of structure tensor
 *
 * @param[in]  input   Grayscale image [width * height]
 * @param[out] sigma1  Larger singular value per pixel [width * height]
 * @param[out] sigma2  Smaller singular value per pixel [width * height]
 * @param[out] angle   Principal orientation in radians [width * height]
 * @param[in]  width   Image width
 * @param[in]  height  Image height
 */
__kernel void svd(__global const uchar* input,
                  __global float* sigma1,
                  __global float* sigma2,
                  __global float* angle,
                  int width,
                  int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int idx = y * width + x;

    /* Skip border pixels (need 1 pixel for Scharr gradients) */
    if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) {
        sigma1[idx] = 0.0f;
        sigma2[idx] = 0.0f;
        angle[idx] = 0.0f;
        return;
    }

    /* Compute image gradients using Scharr operator */
    float Ix, Iy;
    compute_scharr_gradients(input, x, y, width, &Ix, &Iy);

    /* Build structure tensor M = [Ix*Ix  Ix*Iy]
                                  [Ix*Iy  Iy*Iy]
       This is a symmetric positive semi-definite matrix */
    float Sxx = Ix * Ix;
    float Sxy = Ix * Iy;
    float Syy = Iy * Iy;

    /* Compute SVD of symmetric structure tensor */
    float s1, s2, theta;
    svd_2x2_symmetric(Sxx, Sxy, Syy, &s1, &s2, &theta);

    sigma1[idx] = s1;
    sigma2[idx] = s2;
    angle[idx] = theta;
}

/**
 * @brief SVD-based image patch decomposition with Gaussian weighting
 *
 * Computes SVD of accumulated structure tensor over a window.
 * Uses Gaussian weighting similar to OpenCV's cornerEigenValsVecs.
 *
 * @param[in]  input      Grayscale image [width * height]
 * @param[out] s1_out     Larger singular value per pixel [width * height]
 * @param[out] s2_out     Smaller singular value per pixel [width * height]
 * @param[out] coherence  Anisotropy measure (s1-s2)/(s1+s2) [width * height]
 * @param[in]  width      Image width
 * @param[in]  height     Image height
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

/**
 * @brief Jacobi SVD iteration for general matrices (OpenCV-style)
 *
 * Performs one Jacobi rotation to zero out the largest off-diagonal element.
 * This is the core operation in OpenCV's JacobiSVDImpl.
 *
 * @param[in]  input   Grayscale image [width * height]
 * @param[out] U_out   Left singular vectors, 2x2 flattened [width * height * 4]
 * @param[out] S_out   Singular values [width * height * 2]
 * @param[out] V_out   Right singular vectors, 2x2 flattened [width * height * 4]
 * @param[in]  width   Image width
 * @param[in]  height  Image height
 */
__kernel void svd_jacobi(__global const uchar* input,
                         __global float* U_out,
                         __global float* S_out,
                         __global float* V_out,
                         int width,
                         int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    /* Skip border pixels */
    if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) {
        /* Initialize outputs to identity/zero */
        U_out[idx * 4 + 0] = 1.0f; U_out[idx * 4 + 1] = 0.0f;
        U_out[idx * 4 + 2] = 0.0f; U_out[idx * 4 + 3] = 1.0f;
        S_out[idx * 2 + 0] = 0.0f; S_out[idx * 2 + 1] = 0.0f;
        V_out[idx * 4 + 0] = 1.0f; V_out[idx * 4 + 1] = 0.0f;
        V_out[idx * 4 + 2] = 0.0f; V_out[idx * 4 + 3] = 1.0f;
        return;
    }

    /* Compute gradients */
    float Ix, Iy;
    compute_scharr_gradients(input, x, y, width, &Ix, &Iy);

    /* Structure tensor elements */
    float a = Ix * Ix;
    float b = Ix * Iy;
    float c = Iy * Iy;

    /* For 2x2 symmetric matrix, compute full SVD
     * Eigenvalue decomposition: M = V * D * V^T
     * For symmetric matrices: U = V, so SVD = V * S * V^T */

    float trace = a + c;
    float diff = a - c;
    float discriminant = sqrt(diff * diff * 0.25f + b * b);

    float lambda1 = trace * 0.5f + discriminant;
    float lambda2 = trace * 0.5f - discriminant;

    float s1 = sqrt(fmax(lambda1, 0.0f));
    float s2 = sqrt(fmax(lambda2, 0.0f));

    /* Compute eigenvectors (V matrix columns)
     * For eigenvalue lambda, eigenvector satisfies (M - lambda*I)v = 0 */
    float theta;
    if (fabs(b) > 1e-10f) {
        theta = 0.5f * atan2(2.0f * b, a - c);
    } else if (a >= c) {
        theta = 0.0f;
    } else {
        theta = M_PI_2_F;
    }

    float cos_t = cos(theta);
    float sin_t = sin(theta);

    /* V = [cos(theta) -sin(theta); sin(theta) cos(theta)] */
    V_out[idx * 4 + 0] = cos_t;
    V_out[idx * 4 + 1] = -sin_t;
    V_out[idx * 4 + 2] = sin_t;
    V_out[idx * 4 + 3] = cos_t;

    /* For symmetric matrices, U = V */
    U_out[idx * 4 + 0] = cos_t;
    U_out[idx * 4 + 1] = -sin_t;
    U_out[idx * 4 + 2] = sin_t;
    U_out[idx * 4 + 3] = cos_t;

    /* Singular values */
    S_out[idx * 2 + 0] = s1;
    S_out[idx * 2 + 1] = s2;
}
