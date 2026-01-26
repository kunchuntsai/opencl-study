/**
 * @file harris_corner_1.cl
 * @brief Harris Corner Detection OpenCL kernel implementation
 *
 * Based on OpenCV's cornerHarris implementation.
 * Detects corners in images using the Harris corner detector algorithm.
 *
 * Algorithm (following OpenCV):
 * 1. Compute image gradients Ix, Iy using Sobel/Scharr operator
 * 2. Compute gradient products: Ix^2, Iy^2, Ix*Iy
 * 3. Apply Gaussian/box filter smoothing to gradient products (structure tensor M)
 * 4. Compute Harris response: R = det(M) - k * trace(M)^2
 *    where det(M) = Sxx*Syy - Sxy^2 and trace(M) = Sxx + Syy
 * 5. Apply non-maximum suppression to detect corners
 *
 * OpenCV Reference:
 *   - C++ implementation: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/corner.cpp
 *     (see cornerHarris, cornerEigenValsVecs, calcHarris, calcMinEigenVal)
 *   - OpenCL kernel: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/opencl/corner.cl
 *     (see corner kernel with CORNER_HARRIS and CORNER_MINEIGENVAL modes)
 *   - Sobel/Scharr: https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/deriv.cpp
 *
 * @param input    Input grayscale image (uchar)
 * @param output   Output corner response map (float)
 * @param width    Image width in pixels
 * @param height   Image height in pixels
 * @param k        Harris detector free parameter (typically 0.04-0.06)
 */

/* Local memory tile size for optimized processing */
#define TILE_SIZE 16

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
inline void compute_sobel_gradients(__global const uchar* input,
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
 * @brief Compute Scharr gradients at a pixel (more accurate than Sobel)
 *
 * Uses 3x3 Scharr operator as in OpenCV (aperture_size=-1).
 * Scharr Gx: [-3 0 3; -10 0 10; -3 0 3]
 * Scharr Gy: [-3 -10 -3; 0 0 0; 3 10 3]
 * Normalized by /32 for better numerical range.
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
    /* Scharr horizontal gradient (Gx) - normalized */
    *Ix = 3.0f * ((float)input[(y-1) * width + (x+1)] - (float)input[(y-1) * width + (x-1)])
        + 10.0f * ((float)input[y * width + (x+1)] - (float)input[y * width + (x-1)])
        + 3.0f * ((float)input[(y+1) * width + (x+1)] - (float)input[(y+1) * width + (x-1)]);

    /* Scharr vertical gradient (Gy) - normalized */
    *Iy = 3.0f * ((float)input[(y+1) * width + (x-1)] - (float)input[(y-1) * width + (x-1)])
        + 10.0f * ((float)input[(y+1) * width + x] - (float)input[(y-1) * width + x])
        + 3.0f * ((float)input[(y+1) * width + (x+1)] - (float)input[(y-1) * width + (x+1)]);

    /* Normalize by 32 */
    *Ix *= 0.03125f;
    *Iy *= 0.03125f;
}

/**
 * @brief Harris Corner Detection kernel (OpenCV-style)
 *
 * Implements the Harris corner detector following OpenCV's approach:
 * 1. Compute gradients using Sobel operator
 * 2. Build structure tensor with Gaussian weighting (5x5 window)
 * 3. Compute Harris response: R = det(M) - k * trace(M)^2
 *
 * OpenCV Reference:
 *   - https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/corner.cpp#L127
 *
 * @param[in]  input   Input grayscale image (uchar, size: width * height)
 * @param[out] output  Harris corner response map (float, size: width * height)
 *                     Positive values indicate corners, larger = stronger
 * @param[in]  width   Image width in pixels
 * @param[in]  height  Image height in pixels
 * @param[in]  k       Harris detector sensitivity parameter (typically 0.04-0.06)
 */
__kernel void harris_corner(__global const uchar* input,
                            __global float* output,
                            int width,
                            int height,
                            float k) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int idx = y * width + x;

    /* Skip border pixels (need 3 pixels for gradient + 2 for window) */
    if (x < 3 || x >= width - 3 || y < 3 || y >= height - 3) {
        output[idx] = 0.0f;
        return;
    }

    /* Gaussian weights for 5x5 window (sigma ~= 1.0)
     * OpenCV uses boxFilter by default, but Gaussian gives better results
     * These weights sum to 1.0 */
    const float gauss[5][5] = {
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f}
    };

    /* Accumulate weighted gradient products (structure tensor)
     * M = [Sxx Sxy; Sxy Syy] where Sxx = sum(w * Ix^2), etc. */
    float Sxx = 0.0f;
    float Syy = 0.0f;
    float Sxy = 0.0f;

    for (int wy = -2; wy <= 2; wy++) {
        for (int wx = -2; wx <= 2; wx++) {
            int px = x + wx;
            int py = y + wy;

            /* Compute gradients using Sobel operator (OpenCV default) */
            float Ix, Iy;
            compute_sobel_gradients(input, px, py, width, &Ix, &Iy);

            /* Apply Gaussian weight */
            float w = gauss[wy + 2][wx + 2];

            /* Accumulate structure tensor components */
            Sxx += w * Ix * Ix;
            Syy += w * Iy * Iy;
            Sxy += w * Ix * Iy;
        }
    }

    /* Compute Harris response (OpenCV formula)
     * R = det(M) - k * trace(M)^2
     * det(M) = Sxx * Syy - Sxy^2
     * trace(M) = Sxx + Syy */
    float det = Sxx * Syy - Sxy * Sxy;
    float trace = Sxx + Syy;
    float response = det - k * trace * trace;

    output[idx] = response;
}

/**
 * @brief Harris Corner with Scharr gradients (better accuracy)
 *
 * Same as harris_corner but uses Scharr operator for gradients.
 * Scharr provides better rotational symmetry than Sobel.
 * Equivalent to OpenCV's cornerHarris with ksize=-1.
 *
 * @param[in]  input   Input grayscale image (uchar, size: width * height)
 * @param[out] output  Harris corner response map (float, size: width * height)
 *                     Positive values indicate corners, larger = stronger
 * @param[in]  width   Image width in pixels
 * @param[in]  height  Image height in pixels
 * @param[in]  k       Harris detector sensitivity parameter (typically 0.04-0.06)
 */
__kernel void harris_corner_scharr(__global const uchar* input,
                                   __global float* output,
                                   int width,
                                   int height,
                                   float k) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    if (x < 3 || x >= width - 3 || y < 3 || y >= height - 3) {
        output[idx] = 0.0f;
        return;
    }

    const float gauss[5][5] = {
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f}
    };

    float Sxx = 0.0f;
    float Syy = 0.0f;
    float Sxy = 0.0f;

    for (int wy = -2; wy <= 2; wy++) {
        for (int wx = -2; wx <= 2; wx++) {
            int px = x + wx;
            int py = y + wy;

            float Ix, Iy;
            compute_scharr_gradients(input, px, py, width, &Ix, &Iy);

            float w = gauss[wy + 2][wx + 2];

            Sxx += w * Ix * Ix;
            Syy += w * Iy * Iy;
            Sxy += w * Ix * Iy;
        }
    }

    float det = Sxx * Syy - Sxy * Sxy;
    float trace = Sxx + Syy;
    float response = det - k * trace * trace;

    output[idx] = response;
}

/**
 * @brief Compute minimum eigenvalue (Shi-Tomasi corner detection)
 *
 * Computes the minimum eigenvalue of the structure tensor.
 * This is used in Shi-Tomasi "Good Features to Track" algorithm.
 * min_eigenvalue = (trace - sqrt(trace^2 - 4*det)) / 2
 *
 * OpenCV Reference:
 *   - https://github.com/opencv/opencv/blob/4.x/modules/imgproc/src/corner.cpp#L104
 *
 * @param[in]  input   Input grayscale image (uchar, size: width * height)
 * @param[out] output  Minimum eigenvalue map (float, size: width * height)
 *                     Larger values indicate stronger corners
 * @param[in]  width   Image width in pixels
 * @param[in]  height  Image height in pixels
 */
__kernel void harris_min_eigenval(__global const uchar* input,
                                  __global float* output,
                                  int width,
                                  int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    if (x < 3 || x >= width - 3 || y < 3 || y >= height - 3) {
        output[idx] = 0.0f;
        return;
    }

    const float gauss[5][5] = {
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f}
    };

    float Sxx = 0.0f;
    float Syy = 0.0f;
    float Sxy = 0.0f;

    for (int wy = -2; wy <= 2; wy++) {
        for (int wx = -2; wx <= 2; wx++) {
            int px = x + wx;
            int py = y + wy;

            float Ix, Iy;
            compute_sobel_gradients(input, px, py, width, &Ix, &Iy);

            float w = gauss[wy + 2][wx + 2];

            Sxx += w * Ix * Ix;
            Syy += w * Iy * Iy;
            Sxy += w * Ix * Iy;
        }
    }

    /* Minimum eigenvalue formula (OpenCV style)
     * lambda_min = (Sxx + Syy)/2 - sqrt((Sxx - Syy)^2/4 + Sxy^2) */
    float a = Sxx * 0.5f;
    float b = Sxy;
    float c = Syy * 0.5f;
    float min_eigen = (a + c) - sqrt((a - c) * (a - c) + b * b);

    output[idx] = min_eigen;
}

/**
 * @brief Non-maximum suppression for Harris corners
 *
 * Applies non-maximum suppression to the Harris response map
 * to get discrete corner points. A pixel is marked as corner if:
 * 1. Its response exceeds the threshold, AND
 * 2. It is the local maximum in a 3x3 neighborhood
 *
 * @param[in]  response   Harris response map from harris_corner kernel (float, size: width * height)
 * @param[out] corners    Binary corner map (uchar, size: width * height)
 *                        255 = corner detected, 0 = not a corner
 * @param[in]  width      Image width in pixels
 * @param[in]  height     Image height in pixels
 * @param[in]  threshold  Minimum response value to be considered a corner candidate
 */
__kernel void harris_nms(__global const float* response,
                         __global uchar* corners,
                         int width,
                         int height,
                         float threshold) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    /* Skip border pixels */
    if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) {
        corners[idx] = 0;
        return;
    }

    float val = response[idx];

    /* Check if below threshold */
    if (val < threshold) {
        corners[idx] = 0;
        return;
    }

    /* Check if local maximum in 3x3 neighborhood */
    bool is_max = true;
    for (int dy = -1; dy <= 1 && is_max; dy++) {
        for (int dx = -1; dx <= 1 && is_max; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nidx = (y + dy) * width + (x + dx);
            if (response[nidx] >= val) {
                is_max = false;
            }
        }
    }

    corners[idx] = is_max ? 255 : 0;
}

/**
 * @brief Harris corner with local memory optimization (OpenCV-style)
 *
 * Uses local memory to cache input data for better memory bandwidth.
 * Functionally equivalent to harris_corner but with optimized memory access.
 * Based on OpenCV's corner.cl kernel pattern with tile-based processing.
 *
 * @param[in]  input   Input grayscale image (uchar, size: width * height)
 * @param[out] output  Harris corner response map (float, size: width * height)
 *                     Positive values indicate corners, larger = stronger
 * @param[in]  width   Image width in pixels
 * @param[in]  height  Image height in pixels
 * @param[in]  k       Harris detector sensitivity parameter (typically 0.04-0.06)
 */
__kernel void harris_corner_local(__global const uchar* input,
                                  __global float* output,
                                  int width,
                                  int height,
                                  float k) {
    /* Local memory for input tile + border */
    __local uchar tile[TILE_SIZE + 4][TILE_SIZE + 4];

    int lx = get_local_id(0);
    int ly = get_local_id(1);
    int gx = get_global_id(0);
    int gy = get_global_id(1);

    /* Load data into local memory with border */
    int tile_x = gx - lx - 2;
    int tile_y = gy - ly - 2;

    /* Each thread loads one pixel */
    for (int dy = ly; dy < TILE_SIZE + 4; dy += TILE_SIZE) {
        for (int dx = lx; dx < TILE_SIZE + 4; dx += TILE_SIZE) {
            int src_x = clamp(tile_x + dx, 0, width - 1);
            int src_y = clamp(tile_y + dy, 0, height - 1);
            tile[dy][dx] = input[src_y * width + src_x];
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    if (gx >= width || gy >= height) return;

    int idx = gy * width + gx;

    if (gx < 3 || gx >= width - 3 || gy < 3 || gy >= height - 3) {
        output[idx] = 0.0f;
        return;
    }

    /* Gaussian weights */
    const float gauss[5][5] = {
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f},
        {0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f},
        {0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f}
    };

    float Sxx = 0.0f;
    float Syy = 0.0f;
    float Sxy = 0.0f;

    /* Local coordinates with border offset */
    int cx = lx + 2;
    int cy = ly + 2;

    for (int wy = -2; wy <= 2; wy++) {
        for (int wx = -2; wx <= 2; wx++) {
            int px = cx + wx;
            int py = cy + wy;

            /* Sobel gradients from local memory */
            float Ix = (float)tile[py-1][px+1] - (float)tile[py-1][px-1]
                     + 2.0f * ((float)tile[py][px+1] - (float)tile[py][px-1])
                     + (float)tile[py+1][px+1] - (float)tile[py+1][px-1];

            float Iy = (float)tile[py+1][px-1] - (float)tile[py-1][px-1]
                     + 2.0f * ((float)tile[py+1][px] - (float)tile[py-1][px])
                     + (float)tile[py+1][px+1] - (float)tile[py-1][px+1];

            float w = gauss[wy + 2][wx + 2];

            Sxx += w * Ix * Ix;
            Syy += w * Iy * Iy;
            Sxy += w * Ix * Iy;
        }
    }

    float det = Sxx * Syy - Sxy * Sxy;
    float trace = Sxx + Syy;
    float response = det - k * trace * trace;

    output[idx] = response;
}
