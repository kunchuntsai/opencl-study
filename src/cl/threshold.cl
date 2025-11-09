/**
 * Simple Image Thresholding Kernel
 *
 * This kernel demonstrates basic OpenCL concepts:
 * - Global work items and their IDs
 * - Memory access patterns
 * - Basic image processing algorithm
 *
 * Algorithm: Convert each pixel to either 0 (black) or 255 (white)
 * based on whether it's above or below the threshold value.
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
    int x = get_global_id(0);  // Column index
    int y = get_global_id(1);  // Row index

    // Check bounds to ensure we don't access out-of-range memory
    if (x >= width || y >= height) {
        return;
    }

    // Calculate the linear index in the flattened image array
    // For a 2D image stored row-by-row: index = y * width + x
    int idx = y * width + x;

    // Read the input pixel value
    uchar pixel_value = input[idx];

    // Apply threshold: pixels >= threshold become 255 (white), others become 0 (black)
    output[idx] = (pixel_value >= threshold_value) ? 255 : 0;
}
