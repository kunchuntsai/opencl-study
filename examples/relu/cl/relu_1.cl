/**
 * @file relu_1.cl
 * @brief Relu OpenCL kernel implementation
 *
 * TODO: Add algorithm description here
 *
 * Kernel arguments (must match kernel_args in config/relu.json):
 * @param input  Input image buffer
 * @param output Output image buffer
 * @param width  Image width in pixels
 * @param height Image height in pixels
 */

struct relu_params {
    int relu_v1;
    int relu_v1b;
    float relu_v2;
    float relu_v3;
    int relu_v4;
};

__kernel void relu(__global const uchar* input,
                             __global uchar* output,
                             int width,
                             int height,
                             struct relu_params params
                            ) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int index = y * width + x;

    /* ReLU with threshold from params */
    uchar val = input[index];

    if (val < (uchar)params.relu_v1) {
        val = 0;
    }

    output[index] = val;
}
