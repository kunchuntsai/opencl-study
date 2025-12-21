/**
 * @file relu2.cl
 * @brief Relu OpenCL kernel with single workgroup (1D)
 *
 * Kernel arguments (must match kernel_args in config/relu.json):
 * @param input   Input image buffer
 * @param output  Output image buffer
 * @param width   Image width in pixels
 * @param height  Image height in pixels
 * @param params  Struct with relu parameters
 */

struct relu_params {
    int relu_v1;
    int relu_v1b;
    float relu_v2;
    float relu_v3;
    int relu_v4;
};

__kernel void relu_single_workgroup(__global const uchar* input,
                                    __global uchar* output,
                                    int width,
                                    int height,
                                    struct relu_params params) {
    int gid = get_global_id(0);
    int total_pixels = width * height;

    /* Boundary check */
    if (gid >= total_pixels) return;

    /* ReLU with threshold from params */
    uchar val = input[gid];

    if (val < (uchar)params.relu_v1) {
        val = 0;
    }

    output[gid] = val;
}
