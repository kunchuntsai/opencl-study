/**
 * @file lucas_kanade0.cl
 * @brief Lucas-Kanade Optical Flow OpenCL kernel implementation
 *
 * Based on OpenCV's Lucas-Kanade implementation.
 * Computes dense optical flow between two consecutive frames using the
 * Lucas-Kanade method with iterative refinement.
 *
 * Algorithm (following OpenCV):
 * 1. Compute spatial gradients Ix, Iy using Scharr/Sobel operators
 * 2. Compute temporal gradient It = I2 - I1
 * 3. For each pixel, accumulate structure tensor in a window:
 *    A = [sum_Ix2  sum_IxIy]    b = [-sum_IxIt]
 *        [sum_IxIy sum_Iy2 ]        [-sum_IyIt]
 * 4. Solve 2x2 linear system: A * v = b using Cramer's rule
 * 5. (Optional) Iterative refinement for sub-pixel accuracy
 *
 * OpenCV Reference:
 *   - C++ implementation: https://github.com/opencv/opencv/blob/4.x/modules/video/src/lkpyramid.cpp
 *     (see calcOpticalFlowPyrLK, LKTrackerInvoker, calcScharrDeriv)
 *   - OpenCL kernel: https://github.com/opencv/opencv/blob/4.x/modules/video/src/opencl/pyrlk.cl
 *     (see lkSparse kernel, SetPatch, GetPatch, reduce functions)
 *   - Scharr derivatives: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/deriv.cpp
 *
 * ==============================================================================
 * KERNEL I/O SUMMARY
 * ==============================================================================
 *
 * Kernel: lucas_kanade
 * ----------------------------------------------------------------------------
 *   Input:
 *     - prev_frame  : __global const uchar* [width * height]  - Previous grayscale frame
 *     - curr_frame  : __global const uchar* [width * height]  - Current grayscale frame
 *     - width       : int                                     - Image width
 *     - height      : int                                     - Image height
 *     - window_size : int                                     - Window size (odd, typically 5-21)
 *   Output:
 *     - flow_x      : __global float*       [width * height]  - Horizontal flow (pixels)
 *     - flow_y      : __global float*       [width * height]  - Vertical flow (pixels)
 *
 * Kernel: lucas_kanade_iterative
 * ----------------------------------------------------------------------------
 *   Input:
 *     - prev_frame  : __global const uchar* [width * height]  - Previous grayscale frame
 *     - curr_frame  : __global const uchar* [width * height]  - Current grayscale frame
 *     - width       : int                                     - Image width
 *     - height      : int                                     - Image height
 *     - window_size : int                                     - Window size (odd, typically 5-21)
 *     - max_iters   : int                                     - Max iterations for refinement
 *   Output:
 *     - flow_x      : __global float*       [width * height]  - Horizontal flow (sub-pixel)
 *     - flow_y      : __global float*       [width * height]  - Vertical flow (sub-pixel)
 *
 * Kernel: lucas_kanade_gaussian
 * ----------------------------------------------------------------------------
 *   Input:
 *     - prev_frame  : __global const uchar* [width * height]  - Previous grayscale frame
 *     - curr_frame  : __global const uchar* [width * height]  - Current grayscale frame
 *     - width       : int                                     - Image width
 *     - height      : int                                     - Image height
 *   Output:
 *     - flow_x      : __global float*       [width * height]  - Horizontal flow (Gaussian-weighted)
 *     - flow_y      : __global float*       [width * height]  - Vertical flow (Gaussian-weighted)
 *
 * Kernel: flow_to_polar
 * ----------------------------------------------------------------------------
 *   Input:
 *     - flow_x      : __global const float* [width * height]  - Horizontal flow component
 *     - flow_y      : __global const float* [width * height]  - Vertical flow component
 *     - width       : int                                     - Image width
 *     - height      : int                                     - Image height
 *   Output:
 *     - magnitude   : __global float*       [width * height]  - Flow magnitude (pixels)
 *     - angle       : __global float*       [width * height]  - Flow angle (radians)
 *
 * ==============================================================================
 */

/* Constants for iterative refinement */
#define MAX_ITERATIONS 10
#define CONVERGENCE_THRESHOLD 0.01f
#define MIN_EIGENVAL_THRESHOLD 1e-4f

/**
 * @brief Compute Scharr gradients at a pixel (OpenCV style)
 *
 * Uses 3x3 Scharr operator for better accuracy than central differences.
 * Scharr Gx: [-3 0 3; -10 0 10; -3 0 3]
 * Scharr Gy: [-3 -10 -3; 0 0 0; 3 10 3]
 *
 * OpenCV Reference:
 *   - https://github.com/opencv/opencv/blob/4.x/modules/video/src/lkpyramid.cpp#L79
 *     (calcScharrDeriv function)
 *
 * @param input Input image
 * @param x,y   Pixel coordinates
 * @param width Image width
 * @param Ix    Output: horizontal gradient
 * @param Iy    Output: vertical gradient
 */
inline void compute_scharr_gradients(__global const uchar* input,
                                     int x, int y, int width,
                                     float* Ix, float* Iy) {
    /* Scharr kernel coefficients (unnormalized for better precision) */
    /* Gx = [-3 0 3; -10 0 10; -3 0 3] */
    *Ix = 3.0f * ((float)input[(y-1) * width + (x+1)] - (float)input[(y-1) * width + (x-1)])
        + 10.0f * ((float)input[y * width + (x+1)] - (float)input[y * width + (x-1)])
        + 3.0f * ((float)input[(y+1) * width + (x+1)] - (float)input[(y+1) * width + (x-1)]);

    /* Gy = [-3 -10 -3; 0 0 0; 3 10 3] */
    *Iy = 3.0f * ((float)input[(y+1) * width + (x-1)] - (float)input[(y-1) * width + (x-1)])
        + 10.0f * ((float)input[(y+1) * width + x] - (float)input[(y-1) * width + x])
        + 3.0f * ((float)input[(y+1) * width + (x+1)] - (float)input[(y-1) * width + (x+1)]);
}

/**
 * @brief Bilinear interpolation for sub-pixel sampling
 *
 * Used during iterative refinement for sub-pixel accuracy.
 *
 * OpenCV Reference:
 *   - https://github.com/opencv/opencv/blob/4.x/modules/video/src/opencl/pyrlk.cl
 *     (GetPatch function using read_imagef with CLK_FILTER_LINEAR)
 *
 * @param input Input image
 * @param x,y   Floating-point coordinates
 * @param width Image width
 * @param height Image height
 * @return Interpolated pixel value
 */
inline float bilinear_sample(__global const uchar* input,
                             float x, float y,
                             int width, int height) {
    /* Clamp coordinates to image bounds */
    x = fmax(0.0f, fmin(x, (float)(width - 1)));
    y = fmax(0.0f, fmin(y, (float)(height - 1)));

    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = min(x0 + 1, width - 1);
    int y1 = min(y0 + 1, height - 1);

    float fx = x - (float)x0;
    float fy = y - (float)y0;

    float v00 = (float)input[y0 * width + x0];
    float v01 = (float)input[y0 * width + x1];
    float v10 = (float)input[y1 * width + x0];
    float v11 = (float)input[y1 * width + x1];

    /* Bilinear interpolation */
    float v0 = v00 + fx * (v01 - v00);
    float v1 = v10 + fx * (v11 - v10);
    return v0 + fy * (v1 - v0);
}

/**
 * @brief Dense Lucas-Kanade Optical Flow kernel
 *
 * Computes optical flow for every pixel using the Lucas-Kanade method.
 * Based on OpenCV's implementation with the optical flow constraint:
 *   Ix*vx + Iy*vy + It = 0
 *
 * @param prev_frame  Previous frame
 * @param curr_frame  Current frame
 * @param flow_x      Output: horizontal flow
 * @param flow_y      Output: vertical flow
 * @param width       Image width
 * @param height      Image height
 * @param window_size Window size (odd number, typically 5-21)
 */
__kernel void lucas_kanade(__global const uchar* prev_frame,
                           __global const uchar* curr_frame,
                           __global float* flow_x,
                           __global float* flow_y,
                           int width,
                           int height,
                           int window_size) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int half_win = window_size / 2;
    int idx = y * width + x;

    /* Skip border pixels where we can't compute full window */
    if (x < half_win + 1 || x >= width - half_win - 1 ||
        y < half_win + 1 || y >= height - half_win - 1) {
        flow_x[idx] = 0.0f;
        flow_y[idx] = 0.0f;
        return;
    }

    /* Accumulate structure tensor components over the window
     * A = [sum_Ix2  sum_IxIy]
     *     [sum_IxIy sum_Iy2 ]
     * b = [-sum_IxIt, -sum_IyIt] */
    float A11 = 0.0f;  /* sum_Ix2 */
    float A22 = 0.0f;  /* sum_Iy2 */
    float A12 = 0.0f;  /* sum_IxIy */
    float b1 = 0.0f;   /* -sum_IxIt */
    float b2 = 0.0f;   /* -sum_IyIt */

    for (int wy = -half_win; wy <= half_win; wy++) {
        for (int wx = -half_win; wx <= half_win; wx++) {
            int px = x + wx;
            int py = y + wy;

            /* Compute spatial gradients using Scharr operator (OpenCV style) */
            float Ix, Iy;
            compute_scharr_gradients(prev_frame, px, py, width, &Ix, &Iy);

            /* Temporal gradient It = I2 - I1 */
            float It = (float)curr_frame[py * width + px] -
                       (float)prev_frame[py * width + px];

            /* Accumulate */
            A11 += Ix * Ix;
            A22 += Iy * Iy;
            A12 += Ix * Iy;
            b1 -= Ix * It;
            b2 -= Iy * It;
        }
    }

    /* Solve 2x2 linear system using Cramer's rule
     * [A11 A12] [vx]   [b1]
     * [A12 A22] [vy] = [b2]
     *
     * det(A) = A11*A22 - A12*A12
     * vx = (A22*b1 - A12*b2) / det
     * vy = (A11*b2 - A12*b1) / det */
    float det = A11 * A22 - A12 * A12;

    /* Check for singular matrix (flat region or aperture problem)
     * OpenCV uses minimum eigenvalue threshold */
    float min_eigenval = (A11 + A22 - sqrt((A11 - A22) * (A11 - A22) + 4.0f * A12 * A12)) * 0.5f;

    if (fabs(det) < MIN_EIGENVAL_THRESHOLD || min_eigenval < MIN_EIGENVAL_THRESHOLD) {
        flow_x[idx] = 0.0f;
        flow_y[idx] = 0.0f;
        return;
    }

    /* Compute flow using Cramer's rule */
    float inv_det = 1.0f / det;
    float vx = (A22 * b1 - A12 * b2) * inv_det;
    float vy = (A11 * b2 - A12 * b1) * inv_det;

    flow_x[idx] = vx;
    flow_y[idx] = vy;
}

/**
 * @brief Lucas-Kanade with iterative refinement (OpenCV-style)
 *
 * Adds iterative refinement for sub-pixel accuracy, similar to
 * OpenCV's pyramidal LK implementation.
 *
 * OpenCV Reference:
 *   - https://github.com/opencv/opencv/blob/4.x/modules/video/src/opencl/pyrlk.cl
 *     (lkSparse kernel with iterative Newton-Raphson refinement)
 *   - https://github.com/opencv/opencv/blob/4.x/modules/video/src/lkpyramid.cpp#L350
 *     (LKTrackerInvoker::operator() with iteration loop)
 *
 * @param prev_frame  Previous frame
 * @param curr_frame  Current frame
 * @param flow_x      Output: horizontal flow
 * @param flow_y      Output: vertical flow
 * @param width       Image width
 * @param height      Image height
 * @param window_size Window size
 * @param max_iters   Maximum iterations for refinement
 */
__kernel void lucas_kanade_iterative(__global const uchar* prev_frame,
                                     __global const uchar* curr_frame,
                                     __global float* flow_x,
                                     __global float* flow_y,
                                     int width,
                                     int height,
                                     int window_size,
                                     int max_iters) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int half_win = window_size / 2;
    int idx = y * width + x;

    if (x < half_win + 1 || x >= width - half_win - 1 ||
        y < half_win + 1 || y >= height - half_win - 1) {
        flow_x[idx] = 0.0f;
        flow_y[idx] = 0.0f;
        return;
    }

    /* Step 1: Build structure tensor A from previous frame (constant) */
    float A11 = 0.0f;
    float A22 = 0.0f;
    float A12 = 0.0f;

    /* Store gradients for reuse in iterations */
    for (int wy = -half_win; wy <= half_win; wy++) {
        for (int wx = -half_win; wx <= half_win; wx++) {
            int px = x + wx;
            int py = y + wy;

            float Ix, Iy;
            compute_scharr_gradients(prev_frame, px, py, width, &Ix, &Iy);

            A11 += Ix * Ix;
            A22 += Iy * Iy;
            A12 += Ix * Iy;
        }
    }

    /* Check for trackable feature using minimum eigenvalue */
    float det = A11 * A22 - A12 * A12;
    float min_eigenval = (A11 + A22 - sqrt((A11 - A22) * (A11 - A22) + 4.0f * A12 * A12)) * 0.5f;

    if (fabs(det) < MIN_EIGENVAL_THRESHOLD || min_eigenval < MIN_EIGENVAL_THRESHOLD) {
        flow_x[idx] = 0.0f;
        flow_y[idx] = 0.0f;
        return;
    }

    float inv_det = 1.0f / det;

    /* Step 2: Iterative refinement (Newton-Raphson style) */
    float vx = 0.0f;
    float vy = 0.0f;

    for (int iter = 0; iter < max_iters; iter++) {
        float b1 = 0.0f;
        float b2 = 0.0f;

        for (int wy = -half_win; wy <= half_win; wy++) {
            for (int wx = -half_win; wx <= half_win; wx++) {
                int px = x + wx;
                int py = y + wy;

                /* Gradients from previous frame */
                float Ix, Iy;
                compute_scharr_gradients(prev_frame, px, py, width, &Ix, &Iy);

                /* Sample from current frame at displaced position */
                float sample_x = (float)px + vx;
                float sample_y = (float)py + vy;

                /* Get interpolated value from current frame */
                float I2 = bilinear_sample(curr_frame, sample_x, sample_y, width, height);
                float I1 = (float)prev_frame[py * width + px];

                /* Temporal gradient with current estimate */
                float It = I2 - I1;

                b1 -= Ix * It;
                b2 -= Iy * It;
            }
        }

        /* Update flow estimate */
        float dvx = (A22 * b1 - A12 * b2) * inv_det;
        float dvy = (A11 * b2 - A12 * b1) * inv_det;

        vx += dvx;
        vy += dvy;

        /* Check convergence */
        if (fabs(dvx) < CONVERGENCE_THRESHOLD && fabs(dvy) < CONVERGENCE_THRESHOLD) {
            break;
        }
    }

    flow_x[idx] = vx;
    flow_y[idx] = vy;
}

/**
 * @brief Lucas-Kanade with Gaussian weighting (improved accuracy)
 *
 * Uses Gaussian weighting in the window instead of uniform weights.
 * This gives more weight to pixels near the center.
 *
 * @param prev_frame  Previous frame
 * @param curr_frame  Current frame
 * @param flow_x      Output: horizontal flow
 * @param flow_y      Output: vertical flow
 * @param width       Image width
 * @param height      Image height
 */
__kernel void lucas_kanade_gaussian(__global const uchar* prev_frame,
                                    __global const uchar* curr_frame,
                                    __global float* flow_x,
                                    __global float* flow_y,
                                    int width,
                                    int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    /* Use 5x5 window with Gaussian weights */
    if (x < 3 || x >= width - 3 || y < 3 || y >= height - 3) {
        flow_x[idx] = 0.0f;
        flow_y[idx] = 0.0f;
        return;
    }

    /* Gaussian weights for 5x5 window (sigma ~= 1.0) */
    const float gauss[5][5] = {
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f}
    };

    float A11 = 0.0f;
    float A22 = 0.0f;
    float A12 = 0.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;

    for (int wy = -2; wy <= 2; wy++) {
        for (int wx = -2; wx <= 2; wx++) {
            int px = x + wx;
            int py = y + wy;

            float Ix, Iy;
            compute_scharr_gradients(prev_frame, px, py, width, &Ix, &Iy);

            float It = (float)curr_frame[py * width + px] -
                       (float)prev_frame[py * width + px];

            float w = gauss[wy + 2][wx + 2];

            A11 += w * Ix * Ix;
            A22 += w * Iy * Iy;
            A12 += w * Ix * Iy;
            b1 -= w * Ix * It;
            b2 -= w * Iy * It;
        }
    }

    float det = A11 * A22 - A12 * A12;
    float min_eigenval = (A11 + A22 - sqrt((A11 - A22) * (A11 - A22) + 4.0f * A12 * A12)) * 0.5f;

    if (fabs(det) < MIN_EIGENVAL_THRESHOLD || min_eigenval < MIN_EIGENVAL_THRESHOLD) {
        flow_x[idx] = 0.0f;
        flow_y[idx] = 0.0f;
        return;
    }

    float inv_det = 1.0f / det;
    float vx = (A22 * b1 - A12 * b2) * inv_det;
    float vy = (A11 * b2 - A12 * b1) * inv_det;

    flow_x[idx] = vx;
    flow_y[idx] = vy;
}

/**
 * @brief Compute flow magnitude and angle from flow vectors
 *
 * Utility kernel to convert flow vectors to magnitude and angle.
 * Useful for visualization (HSV color coding).
 *
 * @param flow_x      Input: horizontal flow
 * @param flow_y      Input: vertical flow
 * @param magnitude   Output: flow magnitude
 * @param angle       Output: flow angle in radians
 * @param width       Image width
 * @param height      Image height
 */
__kernel void flow_to_polar(__global const float* flow_x,
                            __global const float* flow_y,
                            __global float* magnitude,
                            __global float* angle,
                            int width,
                            int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    float vx = flow_x[idx];
    float vy = flow_y[idx];

    magnitude[idx] = sqrt(vx * vx + vy * vy);
    angle[idx] = atan2(vy, vx);
}
