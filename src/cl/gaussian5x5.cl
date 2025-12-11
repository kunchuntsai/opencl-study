/**
 * @file gaussian5x5.cl
 * @brief OpenCL kernel for 5x5 Gaussian blur filter
 *
 * This kernel demonstrates convolution-based image processing in OpenCL:
 * - 2D convolution with boundary handling
 * - Weighted averaging for blur effects
 * - Constant memory for filter coefficients
 * - Boundary clamping for edge pixels
 *
 * The kernel applies a 5x5 Gaussian blur to smooth/denoise grayscale images.
 * Gaussian blur is widely used in computer vision for noise reduction and
 * image preprocessing.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

/**
 * @brief 5x5 Gaussian blur convolution kernel
 *
 * Applies a 5x5 Gaussian kernel to each pixel by computing a weighted average
 * of the pixel and its neighbors. The Gaussian kernel weights are designed to
 * give more influence to nearby pixels and less to distant pixels.
 *
 * **Gaussian Kernel (normalized):**
 * ```
 *   1   4   6   4   1
 *   4  16  24  16   4
 *   6  24  36  24   6
 *   4  16  24  16   4
 *   1   4   6   4   1
 * ```
 * Sum = 256, so each coefficient is divided by 256
 *
 * **Algorithm:**
 * - For each pixel (x, y):
 *   - Convolve 5x5 neighborhood with Gaussian kernel
 *   - Handle boundaries by clamping coordinates to image bounds
 *   - Compute weighted sum and normalize
 *
 * **Work Organization:**
 * - Work dimension: 2D
 * - Global work size: [width, height]
 * - Each work item processes one output pixel
 * - No work group synchronization needed
 *
 * **Memory Access Pattern:**
 * - Reads: Each work item reads 25 input pixels (5x5 neighborhood)
 * - Writes: Coalesced (work items in same row write adjacent pixels)
 * - Boundary pixels use clamped coordinates (edge pixels repeated)
 *
 * @param[in]  input   Input grayscale image buffer (read-only)
 * @param[out] output  Output blurred image buffer (write-only)
 * @param[in]  width   Image width in pixels
 * @param[in]  height  Image height in pixels
 *
 * @note Assumes row-major storage: pixel[y][x] at index y*width + x
 * @note Boundary handling: clamp coordinates to [0, width-1] and [0, height-1]
 * @note The kernel is separable but implemented as 2D for clarity
 *
 * Example host-side launch:
 * @code
 * size_t globalWorkSize[2] = {width, height};
 * clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
 * @endcode
 */
__kernel void gaussian_blur_5x5(
    __global const uchar* input,   // Input image data (read-only)
    __global uchar* output,        // Output image data (write-only)
    const int width,               // Image width
    const int height               // Image height
)
{
    // Get the global position in the 2D grid
    int x = get_global_id(0);  // Column index
    int y = get_global_id(1);  // Row index

    // Bounds check
    if (x >= width || y >= height) {
        return;
    }

    // 5x5 Gaussian kernel weights (integer approximation, sum = 256)
    // This avoids floating-point operations for better performance
    // Helper function to get kernel weight at position (ky, kx)
    // Row 0: 1  4  6  4  1
    // Row 1: 4 16 24 16  4
    // Row 2: 6 24 36 24  6
    // Row 3: 4 16 24 16  4
    // Row 4: 1  4  6  4  1

    // Accumulator for weighted sum
    int sum = 0;

    // Convolve the 5x5 neighborhood
    // Kernel is centered at (2, 2), so offsets range from -2 to +2
    for (int ky = 0; ky < 5; ky++) {
        for (int kx = 0; kx < 5; kx++) {
            // Calculate neighbor coordinates with offset
            int nx = x + kx - 2;  // -2, -1, 0, +1, +2
            int ny = y + ky - 2;

            // Clamp coordinates to image boundaries
            // This handles edge cases by repeating edge pixels
            nx = clamp(nx, 0, width - 1);
            ny = clamp(ny, 0, height - 1);

            // Get the pixel value at neighbor position
            int idx = ny * width + nx;
            uchar pixel_value = input[idx];

            // Get kernel weight for this position
            // Using explicit values since OpenCL C may not support const array initialization
            int weight = 1;  // Default weight

            // Center row/column (ky=2 or kx=2)
            if (ky == 2 && kx == 2) weight = 36;
            else if ((ky == 2 && kx == 1) || (ky == 2 && kx == 3) ||
                     (ky == 1 && kx == 2) || (ky == 3 && kx == 2)) weight = 24;
            else if ((ky == 2 && kx == 0) || (ky == 2 && kx == 4) ||
                     (ky == 0 && kx == 2) || (ky == 4 && kx == 2)) weight = 6;
            else if ((ky == 1 && kx == 1) || (ky == 1 && kx == 3) ||
                     (ky == 3 && kx == 1) || (ky == 3 && kx == 3)) weight = 16;
            else if ((ky == 1 && kx == 0) || (ky == 1 && kx == 4) ||
                     (ky == 3 && kx == 0) || (ky == 3 && kx == 4) ||
                     (ky == 0 && kx == 1) || (ky == 0 && kx == 3) ||
                     (ky == 4 && kx == 1) || (ky == 4 && kx == 3)) weight = 4;
            // Corners remain 1

            // Multiply by kernel weight and accumulate
            sum += pixel_value * weight;
        }
    }

    // Normalize by dividing by sum of kernel weights (256)
    // Using bit shift for efficient division by power of 2
    uchar result = (uchar)(sum >> 8);  // Divide by 256

    // Write result to output
    int out_idx = y * width + x;
    output[out_idx] = result;
}
