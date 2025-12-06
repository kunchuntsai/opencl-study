/**
 * Gaussian 5x5 blur kernel with true separable convolution - Two-pass approach
 *
 * This variant implements a true separable convolution in two passes:
 * Pass 1: Horizontal blur (input -> tmp_buffer)
 * Pass 2: Vertical blur (tmp_buffer -> output)
 * This is more efficient than computing the 2D kernel directly.
 *
 * @param input Input image buffer
 * @param output Output image buffer
 * @param tmp_buffer Temporary global buffer for intermediate results
 * @param width Image width
 * @param height Image height
 * @param kernel_x Horizontal 1D Gaussian weights (5 floats)
 * @param kernel_y Vertical 1D Gaussian weights (5 floats)
 */

// Pass 1: Horizontal convolution
__kernel void gaussian5x5_horizontal(__global const uchar* input,
                                    __global uchar* output,
                                    __global uchar* tmp_buffer,
                                    __private unsigned long tmp_buffer_size,
                                    __global uchar* tmp_buffer2,
                                    __private unsigned long tmp_buffer_size2,
                                    int width,
                                    int height,
                                    __global const float* kernel_x,
                                    __global const float* kernel_y) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    float sum = 0.0f;
    float kernel_sum = 0.0f;

    // Apply horizontal 1D convolution
    for (int dx = -2; dx <= 2; dx++) {
        int nx = clamp(x + dx, 0, width - 1);
        float weight = kernel_x[dx + 2];
        kernel_sum += weight;
        sum += input[y * width + nx] * weight;
    }

    tmp_buffer[y * width + x] = convert_uchar_sat(sum / kernel_sum);
}
