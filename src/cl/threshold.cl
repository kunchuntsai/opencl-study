/**
 * @file threshold.cl
 * @brief OpenCL kernel for binary image thresholding
 *
 * This kernel demonstrates fundamental OpenCL concepts for parallel image processing:
 * - 2D global work item indexing (get_global_id)
 * - Memory qualifiers (__global, const)
 * - Bounds checking for safe memory access
 * - Coalesced memory access patterns
 *
 * The kernel applies a simple but useful image processing operation:
 * binary thresholding, which converts grayscale images to black and white.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

/**
 * @brief Binary threshold operation kernel
 *
 * Converts a grayscale image to binary (black/white) by comparing each pixel
 * against a threshold value. This operation is embarrassingly parallel - each
 * pixel can be processed independently.
 *
 * **Algorithm:**
 * - If pixel value >= threshold: output = 255 (white)
 * - If pixel value < threshold: output = 0 (black)
 *
 * **Work Organization:**
 * - Work dimension: 2D
 * - Global work size: [width, height]
 * - Each work item processes one pixel
 * - No local memory or work group synchronization needed
 *
 * **Memory Access Pattern:**
 * - Reads: Coalesced (work items in same row read adjacent pixels)
 * - Writes: Coalesced (work items in same row write adjacent pixels)
 * - Access: input[idx], output[idx] where idx = y * width + x
 *
 * @param[in]  input           Input grayscale image buffer (read-only)
 * @param[out] output          Output binary image buffer (write-only)
 * @param[in]  width           Image width in pixels
 * @param[in]  height          Image height in pixels
 * @param[in]  threshold_value Threshold value (0-255)
 *
 * @note Assumes row-major storage: pixel[y][x] at index y*width + x
 * @note Bounds checking prevents out-of-range access when work size > image size
 * @note No work group size requirements - any configuration works
 *
 * Example host-side launch:
 * @code
 * size_t globalWorkSize[2] = {width, height};
 * clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
 * @endcode
 */
__kernel void threshold_image(
    __global const uchar* input,   // Input image data (read-only)
    __global uchar* output,        // Output image data (write-only)
    const int width,               // Image width
    const int height,              // Image height
    const uchar threshold_value    // Threshold value (0-255)
)
{
    // Get the global position in the 2D grid
    // Each work item processes one pixel
    int x = get_global_id(0);  // Column index (0 to width-1)
    int y = get_global_id(1);  // Row index (0 to height-1)

    // Bounds check to ensure we don't access out-of-range memory
    // This handles cases where global work size might be larger than image
    if (x >= width || y >= height) {
        return;
    }

    // Calculate the linear index in the flattened image array
    // For a 2D image stored row-by-row: index = y * width + x
    int idx = y * width + x;

    // Read the input pixel value (coalesced read)
    uchar pixel_value = input[idx];

    // Apply threshold: pixels >= threshold become 255 (white), others become 0 (black)
    // This is a simple conditional that avoids branching divergence
    output[idx] = (pixel_value >= threshold_value) ? 255 : 0;
}
