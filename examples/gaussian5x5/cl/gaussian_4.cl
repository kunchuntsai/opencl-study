/**
 * Gaussian blur kernel with custom scalar parameters
 *
 * Demonstrates custom scalars feature:
 * - sigma (float): Gaussian standard deviation
 * - kernel_radius (int): Radius of kernel (2 for 5x5, 3 for 7x7)
 * - normalize (int): Whether to normalize weights (1=yes, 0=no)
 *
 * Kernel weights are computed on-the-fly from sigma rather than
 * being loaded from external buffer files.
 *
 * @param input Input image buffer
 * @param output Output image buffer
 * @param width Image width
 * @param height Image height
 * @param sigma Gaussian standard deviation (controls blur strength)
 * @param kernel_radius Kernel radius (kernel size = 2*radius + 1)
 * @param normalize Whether to normalize output (1=yes, 0=no)
 */
__kernel void gaussian_custom(__global const uchar* input,
                              __global uchar* output,
                              int width,
                              int height,
                              float sigma,
                              int kernel_radius,
                              int normalize) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    float sum = 0.0f;
    float weight_sum = 0.0f;

    /* Precompute Gaussian coefficient: -1 / (2 * sigma^2) */
    float inv_2sigma2 = -1.0f / (2.0f * sigma * sigma);

    /* Apply NxN convolution with on-the-fly weight computation */
    for (int dy = -kernel_radius; dy <= kernel_radius; dy++) {
        for (int dx = -kernel_radius; dx <= kernel_radius; dx++) {
            /* Clamp coordinates to image bounds */
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);

            /* Compute Gaussian weight: exp(-(dx^2 + dy^2) / (2*sigma^2)) */
            float dist_sq = (float)(dx * dx + dy * dy);
            float weight = exp(dist_sq * inv_2sigma2);

            weight_sum += weight;
            sum += input[ny * width + nx] * weight;
        }
    }

    /* Normalize if requested, otherwise just clamp */
    if (normalize != 0) {
        output[y * width + x] = convert_uchar_sat(sum / weight_sum);
    } else {
        output[y * width + x] = convert_uchar_sat(sum);
    }
}
