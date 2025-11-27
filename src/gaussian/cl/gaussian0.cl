/**
 * Gaussian 5x5 blur kernel with external weights buffer
 *
 * This version demonstrates using CreateBuffersFunc to provide kernel weights
 * via a buffer instead of hardcoding them in the kernel.
 *
 * Benefits of using a buffer:
 * - Weights can be changed at runtime without recompiling the kernel
 * - Reduces kernel code size and register pressure
 * - Better cache utilization for large kernels
 * - Weights are shared across all work items
 *
 * @param input Input image buffer
 * @param output Output image buffer
 * @param weights Pre-computed Gaussian weights (25 floats for 5x5 kernel)
 * @param width Image width
 * @param height Image height
 */
__kernel void gaussian5x5(__global const uchar* input,
                          __global uchar* output,
                          __global const float* weights,
                          int width,
                          int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    float sum = 0.0f;
    int idx = 0;

    // Apply 5x5 convolution using weights from buffer
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);

            // Read weight from global buffer instead of local array
            float weight = weights[idx++];
            sum += input[ny * width + nx] * weight;
        }
    }

    output[y * width + x] = convert_uchar_sat(sum / 256.0f);
}
