/**
 * Gaussian 5x5 blur kernel with separable convolution
 *
 * Uses 1D kernels (kernel_x and kernel_y) loaded from external files.
 * The 2D Gaussian is computed as the outer product of 1D kernels.
 *
 * @param input Input image buffer
 * @param output Output image buffer
 * @param tmp_buffer Temporary global buffer for intermediate results (300MB)
 * @param width Image width
 * @param height Image height
 * @param kernel_x Horizontal 1D Gaussian weights (5 floats)
 * @param kernel_y Vertical 1D Gaussian weights (5 floats)
 */
__kernel void gaussian5x5(__global const uchar* input,
                          __global uchar* output,
                          int width,
                          int height,
                          __global const float* kernel_x,
                          __global const float* kernel_y) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    float sum = 0.0f;
    float kernel_sum = 0.0f;

    // Apply 5x5 convolution using separable kernels
    // 2D weight = kernel_y[i] * kernel_x[j]
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);

            // Compute 2D weight as outer product of 1D kernels
            float weight = kernel_y[dy + 2] * kernel_x[dx + 2];
            kernel_sum += weight;

            sum += input[ny * width + nx] * weight;
        }
    }

    output[y * width + x] = convert_uchar_sat(sum / kernel_sum);
}
